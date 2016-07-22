find . -name \*-conf.d -exec rm -rf {} +
find . -name \*.[od] -exec rm -f {} +
find . -name \*.exe -exec rm -f {} +
find . -name \*.log.new -exec rm -f {} +
find . -name \*.log.valgrind\* -exec rm -f {} +
find . -name \*.log -exec rm -f {} +
rm -rf exe/protocol/{gen,messages}
