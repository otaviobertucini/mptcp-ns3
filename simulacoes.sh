#!/bin/bash

# Terminal colors
declare -r WHITE='\033[0m'
declare -r GRAY='\033[0;38m'
declare -r BLUE='\033[0;34m'
declare -r CYAN='\033[0;36m'
declare -r YELLOW='\033[0;33m'
declare -r GREEN='\033[0;32m'
declare -r RED='\033[0;31m'
declare -r MAGENTA='\033[0;35m'

declare -a colors=( $GRAY
                    $BLUE
                    $CYAN
                    $YELLOW
                    $GREEN
                    $RED
                    $MAGENTA
)


declare -i numRep=1
declare -i simTime=50


# Default data file name
declare DATAFILE=trace_
declare PLOTFILE="plotfile"
declare LABEL=""
# Path of ns3 directory for running waf
declare NS3_PATH=~/git/ns3-mpaqm/


Run(){
    declare -l fileHeader="Delta Goodput"

    # Create data file, optionally specified by user
    if [ "${1}" != "" ]; then
        DATAFILE=$1
    else
        echo -e "Prefix of trace file is ${DATAFILE}"
    fi

    # Taxas de transferencia
    declare -a dataRate=(
                          "1.0"
                        )
    # Delay Range 100-300
    declare -a delayRate=(  "5"
                            "10"
                            "100"
                            "300"
                            "500"
                            )

    echo -e "${BLUE} Running SANIDADE Simluation... ${GRAY}\n"
    c=0
    # Run each combination of settings
    for (( delay=0; delay<${#delayRate[@]}; ++delay ))
    do
        for (( rate=0; rate<${#dataRate[@]}; ++rate ))
            do
                traceFile=${DATAFILE}
                traceFile+=${delayRate[$delay]}"_"
                traceFile+=${dataRate[$rate]}
                traceFile+=".dat"
                touch ${traceFile}
                echo -e "${GRAY} ${fileHeader}"
                # color index

                # execution for delta 0 ... 80
                for ((i=0; i <= $numRep ; i++))
                    do
                            echo -e "${colors[c++]} ./waf --run scratch/sanidade --dataRate=${dataRate[$rate]} --delta=${i} --delay=${delayRate[$delay]} --filename=${traceFile}"

                            ./waf --run "scratch/sanidade \
                                    --dataRate=${dataRate[$rate]} \
                                    --delta=${i} \
                                    --delay=${delayRate[$delay]} \
                                    --simTime=${simTime} \
                                    --filename=${traceFile}" > output.dat 2>&1
                            if ((c > ${#colors[@]}))
                            then
                                c=1
                            fi
                    done
                echo -e "${WHITE}"
            done
    done

    echo -e "${RED}Finished simulation! ${WHITE}\n"
}

Clean(){

cd ${NS3_PATH}

    xmlCount=`find ./ -maxdepth 1 -name "*.xml" | wc -l`
    pcapCount=`find ./ -maxdepth 1 -name "*.pcap" | wc -l`
    datCount=`find ./ -maxdepth 1 -name "*.dat" | wc -l`
    pltCount=`find ./ -maxdepth 1 -name "*.plt" | wc -l`
    plotCount=`find ./ -maxdepth 1 -name "*.plot" | wc -l`

    if [ "$1" == "" ]; then
        if [ ${xmlCount} != 0 ]; then
            rm *.xml
            echo "Removed ${xmlCount} .xml files"
        else
            echo "No .xml files found"
        fi

        if [ ${datCount} != 0 ]; then
            rm *.dat
            echo "Removed ${datCount} .dat files"
        else
            echo "No .dat files found"
        fi

        if [ ${pcapCount} != 0 ]; then
            rm *.pcap
            echo "Removed ${pcapCount} .pcap files"
        else
            echo "No .pcap files found"
        fi

        if [ ${pltCount} != 0 ]; then
            rm *.plt
            echo "Removed ${pltCount} .plt files"
        else
            echo "No .plt files found"
        fi
        if [ ${plotCount} != 0 ]; then
            rm *.plot
            echo "Removed ${plotCount} .plot files"
        else
            echo "No .plot files found"
        fi

    else
        count=`find ./ -maxdepth 1 -name "*${1}" | wc -l`
        if [ ${count} != 0 ]; then
            rm *.${1}
            echo "Removed ${count} .${1} files"
        else
            echo "No .${1} files found"
        fi
    fi

}

# Print usage instructions
ShowUsage()
{
    echo -e "${GRAY}\n"
    echo "Script to run Network Simulator 3 simulations"
    echo "Usage: ./simulacoes.sh <COMMAND> [OPTION]"
    echo "Commands:"
    echo "   -r|--run                     Run simulation"
    echo "       OPTIONS: dataFileName    Run simulation and save data in file named <dataFileName>"
    echo "   -c|--clean                   Clean out the .xml animation and .data files"
    echo "       OPTIONS: file extension  Clean out .<file extension> files"
    echo "   -d|--dplot                   Create data set and plot gnuplot graph"
    echo "   -p|--plot                    Plot gnuplot graph from data set file"
    echo "       OPTIONS: data set        Data set file with 5 columns"
    echo "   -h|--help                    Show this help message"
    echo "Examples:"
    echo "    ./run-p1.sh -r"
    echo "    ./run-p1.sh -r trace_"
    echo "    ./run-p1.sh -c"
    echo "    ./run-p1.sh -c xml"
    echo -e "${WHITE}\n"
}

Plot(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    else
        echo -e "Plot file is ${PLOTFILE}"
    fi
    echo "
        set encoding utf8
        set style data lines
        set grid

        set xlabel 'Delay in ms, ra=${LABEL}ms'
        set ylabel 'Normalized goodput'

        set xrange [0:80]
        set yrange [0:2.5]

        set terminal postscript eps enhanced color font 'Helvetica,22'
        #set terminal png size 900, 300

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        #set output "plot.eps"
        #set title "Throughput vs Increasing RTT of path"

        #table name below graph(naming curve by colour)
        set key right bottom samplen 4

#set fit quiet

        #tamanho dos pontos
        set pointsize 2

        plot '${PLOTFILE}' every 4 using 1:2 with linespoints title '0.5 Mbps' linetype 1 pt 8 linecolor rgb '#001492',\
        '' every 4 u 1:3 w lp t '1 Mbps' lt 1 pt 2 lc rgb '#00F300',\
        '' every 4 u 1:4 w lp t '5 Mbps' lt 2 pt 12 lc rgb '#FF0000',\
        '' every 4 u 1:5 w lp t '10 Mbps' lt 2 pt 1 lc rgb '#696969'
    " | gnuplot
    rm ${PLOTFILE}
}

CreateDataSet(){

    #Gera timestamp para criar arquivos diferentes a cada execucao
    timeplot="`date '+%s'`"
    #Guarda nome temporariamente
    PLOT=${PLOTFILE}
    count=`find ./ -maxdepth 1 -name "trace_100*" | wc -l`
    if [ ${count} != 0 ]; then
      LABEL="100"
      PLOTFILE=${PLOTFILE}"-"$timeplot".100.plot"
      echo ${PLOTFILE}
      echo "Generate Plot "
      gawk '{printf("%.4f\n",$1 )}' trace_100_0.5.dat > ycol.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_100_0.5.dat > xcol1.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_100_1.0.dat > xcol2.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_100_5.0.dat > xcol3.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_100_10.0.dat > xcol4.bfs

      paste ycol.bfs xcol1.bfs xcol2.bfs xcol3.bfs xcol4.bfs > ${PLOTFILE}
      #Chama funcao plot
      Plot
      #remove arquivos temporarios
      rm *.bfs
      PLOTFILE=${PLOT}

    fi

    count=`find ./ -maxdepth 1 -name "trace_200*" | wc -l`
    if [ ${count} != 0 ]; then
      LABEL="200"
      PLOTFILE=${PLOTFILE}"-"$timeplot".200.plot"
      echo ${PLOTFILE}
      echo "Generate Plot "
      gawk '{printf("%.4f\n",$1 )}' trace_200_0.5.dat > ycol.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_200_0.5.dat > xcol1.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_200_1.0.dat > xcol2.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_200_5.0.dat > xcol3.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_200_10.0.dat > xcol4.bfs

      paste ycol.bfs xcol1.bfs xcol2.bfs xcol3.bfs xcol4.bfs > ${PLOTFILE}
      #Chama funcao plot
      Plot
      #remove arquivos temporarios
      rm *.bfs
      PLOTFILE=${PLOT}

    fi

    count=`find ./ -maxdepth 1 -name "trace_300*" | wc -l`
    if [ ${count} != 0 ]; then
      LABEL="300"
      PLOTFILE=${PLOTFILE}"-"$timeplot".300.plot"
      echo ${PLOTFILE}
      echo "Generate Plot "
      gawk '{printf("%.4f\n",$1 )}' trace_300_0.5.dat > ycol.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_300_0.5.dat > xcol1.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_300_1.0.dat > xcol2.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_300_5.0.dat > xcol3.bfs
      gawk '{g = $2; a=$3 * 100/($3+$4); b=$4 * 100/($3+$4); gg=$5*8/500/(1024); aa=gg*a/100;bb=gg*b/100; printf("%.6f\n",gg/aa)}' trace_300_10.0.dat > xcol4.bfs

      paste ycol.bfs xcol1.bfs xcol2.bfs xcol3.bfs xcol4.bfs > ${PLOTFILE}
      #Chama funcao plot
      Plot
      #remove arquivos temporarios
      rm *.bfs
      PLOTFILE=${PLOT}

    fi


}

main()
{
    case "$1" in
        '-r'|'--run' )
            Run $2
        ;;

        '-c'|'--clean' )
            Clean $2
        ;;
        '-d'|'--dplot' )
            CreateDataSet
        ;;
        '-p'|'--plot' )
            Plot $2
        ;;

        *)
            ShowUsage
            exit 1
        ;;
    esac

    exit 0
}

main "$@"
