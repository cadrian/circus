kill $lighttpd_pid
kill $server_pid

{
    echo '----------------------------------------------------------------'
    for log in env access error breakage valgrind; do
        test -r $LOG/$log.log && {
            echo $log
            cat $LOG/$log.log
            echo '----------------------------------------------------------------'
        }
    done
} >$RUN

rm -rf $DIR
