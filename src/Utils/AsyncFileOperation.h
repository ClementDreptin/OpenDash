#pragma once

#include <XexUtils.h>
#include <cstdint>
#include <string>
#include <xtl.h>

class AsyncFileOperation
{
public:
    typedef enum _Status
    {
        Status_Running,
        Status_Succeeded,
        Status_Failed,
        Status_Cancelled,
    } Status;

    struct Progress
    {
        Progress();

        Status CurrentStatus;
        size_t TotalFileCount;
        size_t ProcessedFileCount;
        XexUtils::Fs::Path CurrentFilePath;
        uint64_t CurrentFileSize;
        uint64_t CurrentFileBytesTransferred;
        std::string ErrorMessage;
    };

    AsyncFileOperation();

    ~AsyncFileOperation();

    void Copy(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void Move(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void RequestCancel();

    Progress GetProgress() const;

private:
    HANDLE m_ThreadHandle;
    bool m_CancelRequested;
    XexUtils::Fs::Path m_Source;
    XexUtils::Fs::Path m_Destination;
    Progress m_Progress;
    mutable CRITICAL_SECTION m_ProgressLock;

    struct OperationCancelledException
    {
    };

    void CopyFile(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void CopyDir(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void MoveFile(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void MoveDirAcrossDevices(const XexUtils::Fs::Path &source, const XexUtils::Fs::Path &destination);

    void Fail(const std::string &errorMessage);

private:
    static DWORD WINAPI CopyHandler(void *pArgs);

    static DWORD WINAPI MoveHandler(void *pArgs);

    static DWORD WINAPI ProgressCallback(
        LARGE_INTEGER totalFileSize,
        LARGE_INTEGER totalBytesTransferred,
        LARGE_INTEGER streamSize,
        LARGE_INTEGER streamBytesTransferred,
        DWORD streamNumber,
        DWORD callbackReason,
        HANDLE sourceFileHandle,
        HANDLE destinationFileHandle,
        void *pData
    );

private:
    AsyncFileOperation(const AsyncFileOperation &);
    AsyncFileOperation &operator=(const AsyncFileOperation &);
};
