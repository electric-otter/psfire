void ReadMft(PCWSTR szVolume)
{
    HANDLE hVolume = CreateFileW(szVolume, FILE_READ_DATA, FILE_SHARE_VALID_FLAGS, 0, OPEN_EXISTING, FILE_OPEN_FOR_BACKUP_INTENT, 0);

    if (hVolume != INVALID_HANDLE_VALUE)
    {
        NTFS_VOLUME_DATA_BUFFER nvdb;

        OVERLAPPED ov = {};

        if (DeviceIoControl(hVolume, FSCTL_GET_NTFS_VOLUME_DATA, 0, 0, &nvdb, sizeof(nvdb), 0, &ov))
        {
            NTFS_FILE_RECORD_INPUT_BUFFER nfrib;

            nfrib.FileReferenceNumber.QuadPart = nvdb.MftValidDataLength.QuadPart / nvdb.BytesPerFileRecordSegment - 1;

            ULONG cb = __builtin_offsetof(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer[nvdb.BytesPerFileRecordSegment]);

            PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)alloca(cb);

            do 
            {
                if (!DeviceIoControl(hVolume, FSCTL_GET_NTFS_FILE_RECORD, 
                    &nfrib, sizeof(nfrib), pnfrob, cb, 0, &ov))
                {
                    break;
                }
                
                // pnfrob->FileRecordBuffer :
                // here pnfrob->FileReferenceNumber FileRecord

            } while (0 <= (nfrib.FileReferenceNumber.QuadPart = pnfrob->FileReferenceNumber.QuadPart - 1));

            ReadMft2(szVolume, hVolume, &nvdb);
        }

        CloseHandle(hVolume);
    }
}

void ReadMft2(PCWSTR szVolume, HANDLE hVolume, PNTFS_VOLUME_DATA_BUFFER nvdb)
{
    static PCWSTR MFT = L"\\$MFT";

    static STARTING_VCN_INPUT_BUFFER vcn {};

    static volatile UCHAR guz;

    PVOID stack = alloca(guz);

    union {
        PVOID buf;
        PWSTR lpFileName;
        PRETRIEVAL_POINTERS_BUFFER rpb;
    };

    buf = alloca(wcslen(szVolume) * sizeof(WCHAR) + sizeof(MFT));

    wcscat(wcscpy(lpFileName, szVolume), MFT);

    HANDLE hFile = CreateFileW(lpFileName, 0, FILE_SHARE_VALID_FLAGS, 0, OPEN_EXISTING, FILE_OPEN_FOR_BACKUP_INTENT, 0);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        OVERLAPPED ov{};

        ULONG cb = RtlPointerToOffset(buf, stack), rcb, ExtentCount = 2;

        do 
        {
            rcb = __builtin_offsetof(RETRIEVAL_POINTERS_BUFFER, Extents[ExtentCount]);

            if (cb < rcb)
            {
                cb = RtlPointerToOffset(buf = alloca(rcb - cb), stack);
            }

            if (DeviceIoControl(hFile, FSCTL_GET_RETRIEVAL_POINTERS, &vcn, sizeof(vcn), buf, cb, 0, &ov))
            {
                if (rpb->Extents->Lcn.QuadPart != nvdb->MftStartLcn.QuadPart)
                {
                    __debugbreak();
                }

                if (ExtentCount = rpb->ExtentCount)
                {
                    auto Extents = rpb->Extents;

                    ULONG BytesPerCluster = nvdb->BytesPerCluster;
                    ULONG BytesPerFileRecordSegment = nvdb->BytesPerFileRecordSegment;

                    LONGLONG StartingVcn = rpb->StartingVcn.QuadPart, NextVcn, len;

                    PVOID FileRecordBuffer = alloca(BytesPerFileRecordSegment);

                    do 
                    {
                        NextVcn = Extents->NextVcn.QuadPart;
                        len = NextVcn - StartingVcn, StartingVcn = NextVcn;

                        DbgPrint("%I64x %I64x\n", Extents->Lcn.QuadPart, len);

                        if (Extents->Lcn.QuadPart != -1)
                        {
                            Extents->Lcn.QuadPart *= BytesPerCluster;
                            ov.Offset = Extents->Lcn.LowPart;
                            ov.OffsetHigh = Extents->Lcn.HighPart;

                            // read 1 record
                            ReadFile(hVolume, FileRecordBuffer, BytesPerFileRecordSegment, 0, &ov);
                        }

                    } while (Extents++, --ExtentCount);
                }
                break;
            }

            ExtentCount <<= 1;

        } while (GetLastError() == ERROR_MORE_DATA);

        CloseHandle(hFile);
    }
}

