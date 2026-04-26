CC = gcc
CFLAGS = -I"C:\msys64\ucrt64\include"
LDFLAGS = -L"C:\msys64\ucrt64\lib"
LIBS = "C:\msys64\ucrt64\lib\libcurl.dll.a"

all:
	$(CC) write_to_fast_api.c -o write_to_fast_api.exe -lcurl

clean:
	rm -f *.o write_to_fast_api.exe
