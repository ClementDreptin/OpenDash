#include <XexUtils.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <xtl.h>

#include "../Core/Exceptions.h"
#include "AsyncFileOperation.h"

static const size_t s_TansferBufferSize = 4 * 1024 * 1024; // 4 MB

AsyncFileOperation::Progress::Progress()
    : CurrentStatus(Status_Running), TotalFileCount(0), ProcessedFileCount(0), CurrentFileSize(0), CurrentFileBytesTransferred(0)
{
}

AsyncFileOperation::AsyncFileOperation()
    : m_ThreadHandle(nullptr), m_CancelRequested(false)
{
    InitializeCriticalSection(&m_ProgressLock);
}

AsyncFileOperation::~AsyncFileOperation()
{
    // Wait for the current operation to finish if needed.
    if (m_ThreadHandle != nullptr)
    {
        WaitForSingleObject(m_ThreadHandle, INFINITE);
        CloseHandle(m_ThreadHandle);
    }

    DeleteCriticalSection(&m_ProgressLock);
}

void AsyncFileOperation::Copy(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    m_Source = source;
    m_Destination = destination;
    m_ThreadHandle = XexUtils::Thread(CopyHandler, this);
}

void AsyncFileOperation::Move(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    m_Source = source;
    m_Destination = destination;
    m_ThreadHandle = XexUtils::Thread(MoveHandler, this);
}

void AsyncFileOperation::RequestCancel()
{
    m_CancelRequested = true;
}

AsyncFileOperation::Progress AsyncFileOperation::GetProgress() const
{
    Progress progress;

    EnterCriticalSection(&m_ProgressLock);
    progress = m_Progress;
    LeaveCriticalSection(&m_ProgressLock);

    return progress;
}

void AsyncFileOperation::CopyFile(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    // We are deliberately not using CopyFileEx nor MoveFileWithProgress, which take a
    // callback to track progression, because the chunk size is very small (~64 KB) and
    // making a lot of syscalls has a noticable overhead. So instead we implement our own
    // buffered copy with a much bigger buffer to keep a balance between frequent progress
    // updates and a limited amount of syscalls.

    // Get the source file size.
    WIN32_FILE_ATTRIBUTE_DATA sourceInfo = {};
    BOOL success = GetFileAttributesEx(source.c_str(), GetFileExInfoStandard, &sourceInfo);
    if (!success)
        throw Exception("[AsyncFileOperation]: Couldn't get the size of %s (%i).", source.Filename().c_str(), GetLastError());

    // Initialize the progress for the current file.
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.CurrentFilePath = source;
    m_Progress.CurrentFileBytesTransferred = 0;
    m_Progress.CurrentFileSize = static_cast<uint64_t>(sourceInfo.nFileSizeHigh) << 32 | static_cast<uint64_t>(sourceInfo.nFileSizeLow);

    // If TotalFileCount is not set, it means we're copying a single file and not a
    // directory, so the file count is just 1.
    if (m_Progress.TotalFileCount == 0)
        m_Progress.TotalFileCount = 1;
    LeaveCriticalSection(&m_ProgressLock);

    // Open the source file for reading.
    HANDLE sourceHandle = CreateFile(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (sourceHandle == INVALID_HANDLE_VALUE)
        throw Exception("[AsyncFileOperation]: Couldn't open %s for reading (%i).", source.Filename().c_str(), GetLastError());

    // Open the destination file for writing.
    HANDLE destinationHandle = CreateFile(destination.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (destinationHandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(sourceHandle);
        throw Exception("[AsyncFileOperation]: Couldn't open %s for writing (%i).", destination.Filename().c_str(), GetLastError());
    }

    // Read the source file in chunks into the transfer buffer.
    uint64_t totalCopied = 0;
    DWORD bytesRead = 0;
    while (ReadFile(sourceHandle, m_TransferBuffer.data(), m_TransferBuffer.size(), &bytesRead, nullptr) && bytesRead > 0)
    {
        // Write the read chunk to the destination file.
        DWORD bytesWritten = 0;
        if (!WriteFile(destinationHandle, m_TransferBuffer.data(), bytesRead, &bytesWritten, nullptr))
        {
            CloseHandle(sourceHandle);
            CloseHandle(destinationHandle);
            throw Exception("[AsyncFileOperation]: Couldn't write into %s (%i).", destination.Filename().c_str(), GetLastError());
        }

        // Keep track of how many bytes have been copied so far.
        totalCopied += bytesWritten;

        // Update the progress.
        EnterCriticalSection(&m_ProgressLock);
        m_Progress.CurrentFileBytesTransferred = totalCopied;
        LeaveCriticalSection(&m_ProgressLock);

        // Stop here if the operation was cancelled.
        if (m_CancelRequested)
            break;
    }

    // Close the files.
    CloseHandle(sourceHandle);
    CloseHandle(destinationHandle);

    // Tell the caller the operation stopped because of a cancel request.
    if (m_CancelRequested)
        throw OperationCancelledException();

    // Update the progress.
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.ProcessedFileCount++;
    LeaveCriticalSection(&m_ProgressLock);
}

void AsyncFileOperation::CopyDir(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    // Create the new directory.
    BOOL success = CreateDirectory(destination.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();

        // It's fine if the new directory already exists, this allows merging the old
        // directory into the new one.
        if (error != ERROR_ALREADY_EXISTS)
            throw Exception("[AsyncFileOperation]: Couldn't create directory %s (%i).", destination.Filename().c_str(), error);
    }

    // List the files to move.
    auto files = XexUtils::Fs::ReadDirectory(source);
    if (!files)
        throw Exception("[AsyncFileOperation]: Couldn't read the files in %s.", source.Filename().c_str());

    // Only count the files that are not directories.
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.TotalFileCount += std::count_if(files->begin(), files->end(), [](const XexUtils::Fs::File &file) {
        return !(file.Attributes & FILE_ATTRIBUTE_DIRECTORY);
    });
    LeaveCriticalSection(&m_ProgressLock);

    // Copy every file from the old directory to the new one and recursively copy the sub directories.
    for (size_t i = 0; i < files->size(); i++)
    {
        const XexUtils::Fs::File &file = (*files)[i];
        const XexUtils::Fs::Path &fileFullPath = file.FullPath;
        XexUtils::Fs::Path filename = fileFullPath.Filename();

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            CopyDir(fileFullPath, destination / filename);
        else
            CopyFile(fileFullPath, destination / filename);
    }
}

void AsyncFileOperation::MoveFile(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    // A move across different drives is actually a copy then a delete.
    if (source.Drive() != destination.Drive())
    {
        CopyFile(source, destination);

        BOOL success = DeleteFile(source.c_str());
        if (!success)
            throw Exception(
                "[AsyncFileOperation]: Couldn't delete %s after moving it (%i).",
                source.Filename().c_str(),
                GetLastError()
            );

        return;
    }

    // Moving a file or a directory on the same drive is a simple, fast, atomic operation
    // so it doesn't need a detailed progress.
    BOOL success = ::MoveFile(source.c_str(), destination.c_str());
    if (!success)
    {
        XexUtils::Fs::Path newFilename = destination.Filename();

        uint32_t error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[AsyncFileOperation]: A file or directory called %s already exists.", newFilename.c_str());

        throw Exception("[AsyncFileOperation]: Couldn't move %s (%i).", newFilename.c_str(), error);
    }
}

void AsyncFileOperation::MoveDirAcrossDevices(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination)
{
    // Create the new directory.
    BOOL success = CreateDirectory(destination.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();

        // It's fine if the new directory already exists, this allows merging the old
        // directory into the new one.
        if (error != ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: Couldn't create directory %s (%i).", destination.Filename().c_str(), error);
    }

    // List the files to move.
    auto files = XexUtils::Fs::ReadDirectory(source);
    if (!files)
        throw Exception("[DeviceExplorer]: Couldn't read the files in %s.", source.Filename().c_str());

    // Only count the files that are not directories.
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.TotalFileCount += std::count_if(files->begin(), files->end(), [](const XexUtils::Fs::File &file) {
        return !(file.Attributes & FILE_ATTRIBUTE_DIRECTORY);
    });
    LeaveCriticalSection(&m_ProgressLock);

    // Move every file from the old directory to the new one and recursively move the sub directories.
    for (size_t i = 0; i < files->size(); i++)
    {
        const XexUtils::Fs::File &file = (*files)[i];
        const XexUtils::Fs::Path &fileFullPath = file.FullPath;
        XexUtils::Fs::Path filename = fileFullPath.Filename();

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            MoveDirAcrossDevices(fileFullPath, destination / filename);
        else
            MoveFile(fileFullPath, destination / filename);
    }

    // Delete the now empty source directory.
    success = RemoveDirectory(source.c_str());
    if (!success)
    {
        uint32_t error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            throw Exception("[AsyncFileOperation]: File not found.");

        if (error == ERROR_ACCESS_DENIED)
            throw Exception("[AsyncFileOperation]: Access denied");

        throw Exception("[AsyncFileOperation]: Couldn't delete %s (%i).", source.Filename().c_str(), error);
    }
}

void AsyncFileOperation::Fail(const std::string &errorMessage)
{
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.CurrentStatus = Status_Failed;
    m_Progress.ErrorMessage = errorMessage;
    LeaveCriticalSection(&m_ProgressLock);
}

DWORD WINAPI AsyncFileOperation::CopyHandler(void *pArgs)
{
    AsyncFileOperation *This = static_cast<AsyncFileOperation *>(pArgs);

    // Check if the source file is a directory.
    FILE_ATTRIBUTE sourceFileAttributes = GetFileAttributes(This->m_Source.c_str());
    bool sourceIsDir = sourceFileAttributes != 0xFFFFFFFF && (sourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    try
    {
        // Prepare the transfer buffer.
        This->m_TransferBuffer.resize(s_TansferBufferSize);

        // Perform the copy.
        if (sourceIsDir)
            This->CopyDir(This->m_Source, This->m_Destination);
        else
            This->CopyFile(This->m_Source, This->m_Destination);

        // Update the progress.
        EnterCriticalSection(&This->m_ProgressLock);
        This->m_Progress.CurrentStatus = This->m_CancelRequested ? Status_Cancelled : Status_Succeeded;
        LeaveCriticalSection(&This->m_ProgressLock);
    }
    catch (const OperationCancelledException &)
    {
        // Set the status to cancelled.
        EnterCriticalSection(&This->m_ProgressLock);
        This->m_Progress.CurrentStatus = Status_Cancelled;
        LeaveCriticalSection(&This->m_ProgressLock);
    }
    catch (const Exception &exception)
    {
        This->Fail(exception.what());
    }

    return 0;
}

DWORD WINAPI AsyncFileOperation::MoveHandler(void *pArgs)
{
    AsyncFileOperation *This = static_cast<AsyncFileOperation *>(pArgs);

    // MoveFileEx from Win32 won't move directories across devices, even if the
    // MOVEFILE_COPY_ALLOWED flag is passed, so a manual move of each file is required.
    FILE_ATTRIBUTE sourceFileAttributes = GetFileAttributes(This->m_Source.c_str());
    bool sourceIsDir = sourceFileAttributes != 0xFFFFFFFF && (sourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool destinationIsOnDifferentDevice = This->m_Source.Drive() != This->m_Destination.Drive();

    try
    {
        // Prepare the transfer buffer.
        This->m_TransferBuffer.resize(s_TansferBufferSize);

        // Perform the move.
        if (sourceIsDir && destinationIsOnDifferentDevice)
            This->MoveDirAcrossDevices(This->m_Source, This->m_Destination);
        else
            This->MoveFile(This->m_Source, This->m_Destination);

        // Update the progress.
        EnterCriticalSection(&This->m_ProgressLock);
        This->m_Progress.CurrentStatus = This->m_CancelRequested ? Status_Cancelled : Status_Succeeded;
        LeaveCriticalSection(&This->m_ProgressLock);
    }
    catch (const OperationCancelledException &)
    {
        // Set the status to cancelled.
        EnterCriticalSection(&This->m_ProgressLock);
        This->m_Progress.CurrentStatus = Status_Cancelled;
        LeaveCriticalSection(&This->m_ProgressLock);
    }
    catch (const Exception &exception)
    {
        This->Fail(exception.what());
    }

    return 0;
}
