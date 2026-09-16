#pragma once

#include <XexUtils.h>
#include <cstdint>
#include <string>
#include <vector>
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
    std::vector<uint8_t> m_TransferBuffer;

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

private:
    AsyncFileOperation(const AsyncFileOperation &);
    AsyncFileOperation &operator=(const AsyncFileOperation &);
};
