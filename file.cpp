#include "rar.hpp"

File::File()
{
  hFile=FILE_BAD_HANDLE;
  *FileName=0;
  NewFile=false;
  LastWrite=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  IgnoreReadErrors=false;
  ErrorType=FILE_SUCCESS;
  OpenShared=false;
  AllowDelete=true;
  AllowExceptions=true;
}


File::~File()
{
  if (hFile!=FILE_BAD_HANDLE && !SkipClose)
    if (NewFile)
      Delete();
    else
      Close();
}


void File::operator = (File &SrcFile)
{
  hFile=SrcFile.hFile;
  NewFile=SrcFile.NewFile;
  LastWrite=SrcFile.LastWrite;
  HandleType=SrcFile.HandleType;
  wcsncpyz(FileName,SrcFile.FileName,ASIZE(FileName));
  SrcFile.SkipClose=true;
}


bool File::Open(const wchar *Name,uint Mode)
{
  ErrorType=FILE_SUCCESS;
  FileHandle hNewFile;
  bool OpenShared=File::OpenShared || (Mode & FMF_OPENSHARED)!=0;
  bool UpdateMode=(Mode & FMF_UPDATE)!=0;
  bool WriteMode=(Mode & FMF_WRITE)!=0;

  int flags=UpdateMode ? O_RDWR:(WriteMode ? O_WRONLY:O_RDONLY);
  char NameA[NM];
  WideToChar(Name,NameA,ASIZE(NameA));

  FileHandle handle=rario.open(NameA,flags,0);
  if (handle==FILE_BAD_HANDLE)
    hNewFile=FILE_BAD_HANDLE;
  else
  {
    hNewFile=handle;
  }
  if (hNewFile==FILE_BAD_HANDLE && errno==ENOENT)
    ErrorType=FILE_NOTFOUND;
  NewFile=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  bool Success=hNewFile!=FILE_BAD_HANDLE;
  if (Success)
  {
    hFile=hNewFile;
    wcsncpyz(FileName,Name,ASIZE(FileName));
  }
  return Success;
}


#if !defined(SFX_MODULE)
void File::TOpen(const wchar *Name)
{
  if (!WOpen(Name))
    ErrHandler.Exit(RARX_OPEN);
}
#endif


bool File::WOpen(const wchar *Name)
{
  if (Open(Name))
    return true;
  ErrHandler.OpenErrorMsg(Name);
  return false;
}


bool File::Create(const wchar *Name,uint Mode)
{
  // OpenIndiana based NAS and CIFS shares fail to set the file time if file
  // was created in read+write mode and some data was written and not flushed
  // before SetFileTime call. So we should use the write only mode if we plan
  // SetFileTime call and do not need to read from file.
  bool WriteMode=(Mode & FMF_WRITE)!=0;
  bool ShareRead=(Mode & FMF_SHAREREAD)!=0 || File::OpenShared;
  char NameA[NM];
  WideToChar(Name,NameA,ASIZE(NameA));
  hFile=rario.open(NameA,(O_CREAT|O_TRUNC) | (WriteMode ? O_WRONLY : O_RDWR),0666);
  NewFile=true;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  wcsncpyz(FileName,Name,ASIZE(FileName));
  return hFile!=FILE_BAD_HANDLE;
}


#if !defined(SFX_MODULE)
void File::TCreate(const wchar *Name,uint Mode)
{
  if (!WCreate(Name,Mode))
    ErrHandler.Exit(RARX_FATAL);
}
#endif


bool File::WCreate(const wchar *Name,uint Mode)
{
  if (Create(Name,Mode))
    return true;
  ErrHandler.CreateErrorMsg(Name);
  return false;
}


bool File::Close()
{
  bool Success=true;

  if (hFile!=FILE_BAD_HANDLE)
  {
    if (!SkipClose)
    {
      Success=rario.close(hFile)!=-1;
    }
    hFile=FILE_BAD_HANDLE;
  }
  HandleType=FILE_HANDLENORMAL;
  if (!Success && AllowExceptions)
    ErrHandler.CloseError(FileName);
  return Success;
}


bool File::Delete()
{
  if (HandleType!=FILE_HANDLENORMAL)
    return false;
  if (hFile!=FILE_BAD_HANDLE)
    Close();
  if (!AllowDelete)
    return false;
  return DelFile(FileName);
}


bool File::Rename(const wchar *NewName)
{
  // No need to rename if names are already same.
  bool Success=wcscmp(FileName,NewName)==0;

  if (!Success)
    Success=RenameFile(FileName,NewName);

  if (Success)
    wcscpy(FileName,NewName);

  return Success;
}


bool File::Write(const void *Data,size_t Size)
{
  if (Size==0)
    return true;
  if (HandleType==FILE_HANDLESTD)
  {
    // Cannot use the standard stdout here, because it already has wide orientation.
    if (hFile==FILE_BAD_HANDLE)
    {
      return false;
    }
  }
  bool Success;
  while (1)
  {
    Success=false;
    ssize_t Written=rario.write(hFile,Data,Size);
    Success=Written==Size;
    if (!Success && AllowExceptions && HandleType==FILE_HANDLENORMAL)
    {
      if (ErrHandler.AskRepeatWrite(FileName,false))
      {
        if (Written<Size && Written>0)
          Seek(Tell()-Written,SEEK_SET);
        continue;
      }
      ErrHandler.WriteError(NULL,FileName);
    }
    break;
  }
  LastWrite=true;
  return Success; // It can return false only if AllowExceptions is disabled.
}


int File::Read(void *Data,size_t Size)
{
  int64 FilePos=0; // Initialized only to suppress some compilers warning.

  if (IgnoreReadErrors)
    FilePos=Tell();
  int ReadSize;
  while (true)
  {
    ReadSize=DirectRead(Data,Size);
    if (ReadSize==-1)
    {
      ErrorType=FILE_READERROR;
      if (AllowExceptions)
        if (IgnoreReadErrors)
        {
          ReadSize=0;
          for (size_t I=0;I<Size;I+=512)
          {
            Seek(FilePos+I,SEEK_SET);
            size_t SizeToRead=Min(Size-I,512);
            int ReadCode=DirectRead(Data,SizeToRead);
            ReadSize+=(ReadCode==-1) ? 512:ReadCode;
          }
        }
        else
        {
          if (HandleType==FILE_HANDLENORMAL && ErrHandler.AskRepeatRead(FileName))
            continue;
          ErrHandler.ReadError(FileName);
        }
    }
    break;
  }
  return ReadSize;
}


// Returns -1 in case of error.
int File::DirectRead(void *Data,size_t Size)
{
  if (HandleType==FILE_HANDLESTD)
  {
    hFile=STDIN_FILENO;
  }
  ssize_t ReadSize=rario.read(hFile,Data,Size);
  if (ReadSize==-1)
    return -1;
  return (int)ReadSize;
}


void File::Seek(int64 Offset,int Method)
{
  if (!RawSeek(Offset,Method) && AllowExceptions)
    ErrHandler.SeekError(FileName);
}


bool File::RawSeek(int64 Offset,int Method)
{
  if (hFile==FILE_BAD_HANDLE)
    return true;
  if (Offset<0 && Method!=SEEK_SET)
  {
    Offset=(Method==SEEK_CUR ? Tell():FileLength())+Offset;
    Method=SEEK_SET;
  }
  LastWrite=false;
  if (rario.lseek(hFile,(off_t)Offset,Method)==-1)
    return false;
  return true;
}


int64 File::Tell()
{
  if (hFile==FILE_BAD_HANDLE)
    if (AllowExceptions)
      ErrHandler.SeekError(FileName);
    else
      return -1;
  return rario.lseek(hFile,0,SEEK_CUR);
}


void File::Prealloc(int64 Size)
{
}


byte File::GetByte()
{
  byte Byte=0;
  Read(&Byte,1);
  return Byte;
}


void File::PutByte(byte Byte)
{
  Write(&Byte,1);
}


bool File::Truncate()
{
    return true;
}


void File::Flush()
{
}


void File::SetOpenFileTime(RarTime *ftm,RarTime *ftc,RarTime *fta)
{
}


void File::SetCloseFileTime(RarTime *ftm,RarTime *fta)
{
}


void File::SetCloseFileTimeByName(const wchar *Name,RarTime *ftm,RarTime *fta)
{
}


void File::GetOpenFileTime(RarTime *ft)
{
    memset(ft, 0, sizeof(*ft));
}


int64 File::FileLength()
{
  return rario.fileLength(hFile);
}


bool File::IsDevice()
{
    return false;
}


#ifndef SFX_MODULE
int64 File::Copy(File &Dest,int64 Length)
{
  Array<char> Buffer(0x40000);
  int64 CopySize=0;
  bool CopyAll=(Length==INT64NDF);

  while (CopyAll || Length>0)
  {
    Wait();
    size_t SizeToRead=(!CopyAll && Length<(int64)Buffer.Size()) ? (size_t)Length:Buffer.Size();
    char *Buf=&Buffer[0];
    int ReadSize=Read(Buf,SizeToRead);
    if (ReadSize==0)
      break;
    size_t WriteSize=ReadSize;
    Dest.Write(Buf,WriteSize);
    CopySize+=ReadSize;
    if (!CopyAll)
      Length-=ReadSize;
  }
  return CopySize;
}
#endif
