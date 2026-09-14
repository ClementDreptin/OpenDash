#include <XexUtils.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <xtl.h>

#include "../Core/Exceptions.h"
#include "AsyncFileOperation.h"

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
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.CurrentFilePath = source;

    // If TotalFileCount is not set, it means we're copying a single file and not a
    // directory, so the file count is just 1.
    if (m_Progress.TotalFileCount == 0)
        m_Progress.TotalFileCount = 1;
    LeaveCriticalSection(&m_ProgressLock);

    // Copy the file.
    BOOL success = CopyFileEx(source.c_str(), destination.c_str(), ProgressCallback, this, nullptr, 0);
    if (!success)
    {
        uint32_t error = GetLastError();
        if (error == ERROR_REQUEST_ABORTED)
            throw OperationCancelledException();

        XexUtils::Fs::Path destinationFilename = destination.Filename();
        throw Exception("[AsyncFileOperation]: Couldn't copy %s (%i).", destinationFilename.c_str(), error);
    }

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
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.CurrentFilePath = source;

    // If TotalFileCount is not set, it means we're moving a single file and not a
    // directory, so the file count is just 1.
    if (m_Progress.TotalFileCount == 0)
        m_Progress.TotalFileCount = 1;
    LeaveCriticalSection(&m_ProgressLock);

    // Move the file.
    BOOL success = MoveFileEx(source.c_str(), destination.c_str(), MOVEFILE_COPY_ALLOWED);
    if (!success)
    {
        uint32_t error = GetLastError();
        if (error == ERROR_REQUEST_ABORTED)
            throw OperationCancelledException();

        XexUtils::Fs::Path newFilename = destination.Filename();
        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[AsyncFileOperation]: A file or directory called %s already exists.", newFilename.c_str());

        throw Exception("[AsyncFileOperation]: Couldn't move %s (%i).", newFilename.c_str(), error);
    }

    // Update the progress.
    EnterCriticalSection(&m_ProgressLock);
    m_Progress.ProcessedFileCount++;
    LeaveCriticalSection(&m_ProgressLock);
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

DWORD WINAPI AsyncFileOperation::ProgressCallback(
    LARGE_INTEGER totalFileSize,
    LARGE_INTEGER totalBytesTransferred,
    LARGE_INTEGER streamSize,
    LARGE_INTEGER streamBytesTransferred,
    DWORD streamNumber,
    DWORD callbackReason,
    HANDLE sourceFileHandle,
    HANDLE destinationFileHandle,
    void *pData
)
{
    AsyncFileOperation *This = static_cast<AsyncFileOperation *>(pData);

    // Update the progress.
    EnterCriticalSection(&This->m_ProgressLock);
    This->m_Progress.CurrentFileSize = static_cast<uint64_t>(totalFileSize.QuadPart);
    This->m_Progress.CurrentFileBytesTransferred = static_cast<uint64_t>(totalBytesTransferred.QuadPart);
    LeaveCriticalSection(&This->m_ProgressLock);

    return This->m_CancelRequested ? PROGRESS_CANCEL : PROGRESS_CONTINUE;
}
