#!/bin/bash


Commit(){
	echo -e "Make COMMIT to MASTER .."
	DATEBCK="`date '+%Y-%m-%d-%H-%M-%S'`"
        git commit -m "Backup in ${DATEBCK}"
        git push origin master
	echo "FINISH!"

}

Alt(){
	git add .
	Commit
}
New(){
	git add -u
	Commit
}
All(){
	git add -A
	Commit
}
Pull(){
	git pull
}
ShowUsage()
{
    echo "    ./git update -{c(changed),n(new),A(All)}"
}
main()
{
    case "$1" in
        '-c'|'--changed' )
            Alt $2
        ;;

        '-n'|'--new' )
            New $2
        ;;

	'-p'|'--pull' )
            Pull $2
        ;;

        '-A'|'--All' )
            All
        ;;

        *)
            ShowUsage
            exit 1
        ;;
    esac

    exit 0
}

main "$@"
