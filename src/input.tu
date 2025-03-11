//> math
> file

# main(args vec{string}) int
  //- f = file.open("foo.txt", file._READ_ | file._WRITE_)
  //- f = file.open("foo.txt", file.mode.r)
  //- f = file.open_r("foo.txt")
  //- f = file.open_rp("foo.txt")
  //-f file.FileRead
  //-f file.FileWrite
  //-f file.FileReadWrite
  //-f file.FileWriteAppend
  //-f file.FileReadWriteAppend

  - txt = file.read_text("input.tu")
  print(txt)

  return 42
