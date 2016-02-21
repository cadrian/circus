redo-ifchange exe/protocol/messages
redo-ifchange _test
#TODO: redo executables
redo-ifchange $(dirname $2)/exe/main/server.exe
redo-ifchange $(dirname $2)/exe/main/client_cgi.exe
