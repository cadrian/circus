kill $lighttpd_pid 2>/dev/null
kill $server_pid 2>/dev/null

{
    echo '----------------------------------------------------------------'
    for log in $LOG/*.log; do
        test -r $log && {
            echo "[$(basename $log .log)]"
            echo
            cat $log
            echo '----------------------------------------------------------------'
        }
    done
} >$RUN

echo '----------------------------------------------------------------'
for res in $base.*.res; do
    for s in .in .hdr .res .out; do
        f=${res%.res}$s
        echo "[$(basename $f)]"
        echo
        case $s in
            .hdr)
                grep -v '^Date:' $f
                ;;
            .in)
                cat $f
                echo
                ;;
            *)
                cat $f
                ;;
        esac
        echo '----------------------------------------------------------------'
    done
done

rm -rf $DIR
