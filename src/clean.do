find . -name \*.[od] -exec rm {} +
find . -name \*.exe -exec rm {} +
find . -name \*.log.new -exec rm {} +
rm -rf main/protocol/{gen,messages}
