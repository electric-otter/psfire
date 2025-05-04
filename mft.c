
2
Closed. This question needs to be more focused. It is not currently accepting answers.

Want to improve this question? Update the question so it focuses on one problem only by editing this post.

Closed 6 years ago.

I want to read the MFT of my local system using C. I search for solution all over the internet but nothing found. I hope someone has a tutorial for me or a good explanation with example of code about how to do that. Thanks in advance.

    cwindowswinapi

Share
Edit
Follow
Get updates on questions and answers
asked Feb 9, 2019 at 13:38
Ori's user avatar
Ori
8211 silver badge88 bronze badges

    what sense read $MFT direct ? what is final target ? – 
    RbMm
    Commented Feb 9, 2019 at 14:02

Add a comment
2 Answers
Sorted by:
6

First, we need to open the volume handle with FILE_READ_DATA rights.

Then we need to query NTFS_VOLUME_DATA_BUFFER with FSCTL_GET_NTFS_VOLUME_DATA control code.

From here we got the size of a single MFT record - BytesPerFileRecordSegment, the total size of MFT - MftValidDataLength. So the maximum record count is (MftValidDataLength.QuadPart / BytesPerFileRecordSegment).

The correct way (synchronized with NTFS) will be to read a single record via FSCTL_GET_NTFS_FILE_RECORD.

If you want to read multiple records at once - it is of course possible to read directly from the volume. We have the start LCN for MFT - MftStartLcn. But MFT can have several not continuous fragments. So we need to use FSCTL_GET_RETRIEVAL_POINTERS if we want to get all fragment locations. To convert LCN to the volume offset we need multiply it by BytesPerCluster.

demo code:

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

