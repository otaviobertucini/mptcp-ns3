#!/bin/bash

# Terminal colors
declare -r NOCOLOR='\033[0m'
declare -r WHITE='\033[1;37m'
declare -r GRAY='\033[;37m'
declare -r BLUE='\033[0;34m'
declare -r CYAN='\033[0;36m'
declare -r YELLOW='\033[0;33m'
declare -r GREEN='\033[0;32m'
declare -r RED='\033[0;31m'
declare -r MAGENTA='\033[0;35m'

declare -a colors=( $NOCOLOR
                    $GRAY
                    $BLUE
                    $CYAN
                    $YELLOW
                    $GREEN
                    $RED
                    $MAGENTA
)

declare -i numRep=1
declare -i simTime=60
declare -i logging=1
declare -i pcap=1
declare -i MSS=1458


# Default data file name
declare FAKESIM="n"
declare DATAFILE="bfs"
declare DATAFILE_A="da_.dat"
declare DATAFILE_B="db_.dat"
declare PLOT="plot_"
declare PLOTFILE
declare BACKUP_PLOTS="plots"
declare xLABEL=""
declare yLABEL=""
declare gLABEL=""
declare xRANGE=""
declare yRANGE=""
# Path of ns3 directory for running waf
declare NS3_PATH=~/git/ns3-mpaqm/

Run(){

    # Create data file, optionally specified by user
    if [ "${1}" != "" ]; then
        DATAFILE=$1
    else
        echo -e "Prefix of trace file is ${DATAFILE}"
    fi

    # Taxas de transferencia 0.5 -- 100Mbps
    declare -a dataRate=(  "1"
                           #"2"
                        )
    # Delay Range 1-300
    declare -a delayRate=(  "0.001"
                            #"0.100"
                            #"0.500"
                            )
    # Queue Size 1-1000
    declare -a queueSize=(
                            #"100"
                            "1000"
                            )
    # Queue Size 1-1000
    declare -a bufferSize=(
                              "65536"
                            )
    # Queue Type
    declare -a queueType=(  "DropTail"
                            #"CoDelQueue"
                            #"CoDelQueueLifo"
                            #"Fq_CoDel"
                            )
    # Congestion Control
    declare -a congestionControl=(
                            "RTT_Compensator"
                            #"Linked_Increases"
                            #"Uncoupled_TCPs"
                            #"COUPLED_EPSILON"
                            )


    echo -e "${BLUE}Running DropTail-Codel Simluation... ${GRAY}\n"
    c=0
    # Run each combination of settings
    for (( congestion=0; congestion<${#congestionControl[@]}; ++congestion ))
    do
      for (( qtype=0; qtype<${#queueType[@]}; ++qtype ))
      do
        for (( rate=0; rate<${#dataRate[@]}; ++rate ))
        do
          for (( delay=0; delay<${#delayRate[@]}; ++delay ))
          do
            for (( bsize=0; bsize<${#bufferSize[@]}; ++bsize ))
            do
              for (( qsize=0; qsize<${#queueSize[@]}; ++qsize ))
              do
                fileNamePrefix=${DATAFILE}"_"
                fileNamePrefix+=${congestionControl[$congestion]}"_"
                fileNamePrefix+=${queueType[$qtype]}"_"
                fileNamePrefix+=${dataRate[$rate]}"_"
                fileNamePrefix+=${delayRate[$delay]}"_"
                fileNamePrefix+=${bufferSize[$bsize]}"_"
                fileNamePrefix+=${queueSize[$qsize]}

                # execution for delta 0 ... 30
                for ((i=0; i < $numRep ; i++))
                do
                            NAME="${fileNamePrefix}-${i}"
                            echo -e "${colors[c++]} - Run \"analise-queue-mptcp-v1\" ${NAME}"
                    #IF FAKESIM, RUN IN DIRECTORY OF BACKUP TRACES
                    if [ "${FAKESIM}" != "y" ]; then
                        #fileNamePrefix+=${i}
                        ./waf --run "scratch/analise-queue-mptcp-v1 \
                              --congestionControl=${congestionControl[$congestion]} \
                              --queueType=${queueType[$qtype]} \
                              --dataRate=${dataRate[$rate]} \
                              --delayRate=${delayRate[$delay]} \
                              --bufferSize=${bufferSize[$bsize]} \
                              --queueSize=${queueSize[$qsize]} \
                              --simTime=${simTime} \
                              --logging=${logging} \
                              --pcap=${pcap} \
                              --fileNamePrefix=${fileNamePrefix}-${i}"  > output-${i}.dat 2>&1

                              #Rename Traces
                              RenameTraces $NAME
                    fi
                        PARAMS="${congestionControl[$congestion]} ${queueType[$qtype]} ${dataRate[$rate]} ${delayRate[$delay]} ${bufferSize[$bsize]} ${queueSize[$qsize]} ${NAME}"
                        plotRTT ${PARAMS}
                        plotCWND ${PARAMS}
                        plotTroughput ${PARAMS}

                        if [ "${queueType[$qtype]}" != "DropTail" ]; then
                            #plotCoDelQueueLenght ${PARAMS}
                            plotCoDelDropState ${PARAMS}
                            plotCoDelDropPackets ${PARAMS}
                            plotCoDelSoujorTime ${PARAMS}
                            plotCoDelSoujornFreq ${PARAMS}
                        fi

                        if [ "${queueType[$qtype]}" == "DropTail" ]; then
                            plotDropTailDrops ${PARAMS}
                        fi

                        if ((c > ${#colors[@]}))
                        then
                            c=1
                        fi
                done
                echo -e "${NOCOLOR}"
            done
          done
        done
      done
    done
  done
  if [ "${FAKESIM}" != "y" ]; then
    echo "Moving Traces"
    MoveTraces
  fi
    echo -e "${RED}Finished simulation! ${NOCOLOR}\n"
}

MoveTraces(){
  DIRBACKUP="`date '+%Y-%m-%d-%H-%M-%S'`"
  mkdir -p plots/$DIRBACKUP
  mkdir -p plots/$DIRBACKUP/pdfs
  mkdir -p plots/$DIRBACKUP/plotfiles

  # Move trace files
  for file in *.tr
  do
      mv $file plots/$DIRBACKUP
  done
  # Move PDF files
  for file in *.pdf
  do
      mv $file plots/$DIRBACKUP/pdfs
  done
  # Move PLOT files
  for file in *.plot
  do
      mv $file plots/$DIRBACKUP/plotfiles
  done
}
RenameTraces(){
  # Move trace files with prefixName to folder traces
  for file in *tracer.tr
  do
      mv "$file" "${file/tracer/${1}-}"
  done
}
Clean(){

    cd ${NS3_PATH}

    tmpCount=`find ./ -maxdepth 1 -name "*.tmp" | wc -l`
    pcapCount=`find ./ -maxdepth 1 -name "*.pcap" | wc -l`
    datCount=`find ./ -maxdepth 1 -name "*.dat" | wc -l`
    pltCount=`find ./ -maxdepth 1 -name "*.plt" | wc -l`
    plotCount=`find ./ -maxdepth 1 -name "*.plot" | wc -l`
    trCount=`find ./ -maxdepth 1 -name "*.tr" | wc -l`

    if [ "$1" == "" ]; then
        if [ ${tmpCount} != 0 ]; then
            rm *.tmp
            echo "Removed ${tmpCount} .tmp files"
        else
            echo "No .tmp files found"
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
        if [ ${trCount} != 0 ]; then
            rm *.tr
            echo "Removed ${trCount} .tr files"
        else
            echo "No .tr files found"
        fi

    else
        count=`find ./ -maxdepth 1 -name "*.${1}" | wc -l`
        if [ ${count} != 0 ]; then
            rm *.${1}
            echo "Removed ${count} .${1} files"
        else
            echo "No .${1} files found"
        fi
    fi

}

# Two Y column and thwo X column
gnuYYXX(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    fi

    echo "
        set termoption dashed
        set encoding utf8
        set style data lines
        set grid
        set size 2,1

        set xlabel '${xLABEL}'
        set ylabel '${yLABEL}'
        set xrange ${xRANGE}
        set yrange ${yRANGE}

        set terminal postscript eps enhanced color font 'Helvetica,32'

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        #set title '${gLABEL}'

        #table name below graph(naming curve by colour)
        set key right bottom outside samplen 4
        #estilo das linhas
        set style line 1 lc rgb '#0000FF' lt 1 dashtype 1 lw 0.5 pt 1
        set style line 2 lc rgb '#FF0000' lt 3 dashtype '-' lw 0.5 pt 3
        #set fit quiet

        #tamanho dos pontos
        set pointsize 1.4

        plot  '${DATAFILE_A}' every 1 using 1:2 t 'WiFi' with linespoints ls 1,\
              '${DATAFILE_B}' every 1 u 1:2 t 'LTE' with linespoints ls 2
    " | gnuplot

}
# One Y column and two X column
gnuYXX(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    fi
        echo "
        set termoption dashed
        set encoding utf8
        set style data lines
        set grid nopolar
        set grid xtics nomxtics ytics nomytics noztics nomztics \
        nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics
        set grid layerdefault   lt 0 linewidth 0.500,  lt 0 linewidth 0.500
        set size 2,1

        set xlabel '${xLABEL}'
        set ylabel '${yLABEL}'
        set xrange ${xRANGE}
        set yrange ${yRANGE}

        set terminal postscript eps enhanced color font 'Helvetica,32'

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        #set title '${gLABEL}'

        #table name below graph(naming curve by colour)
        set key right bottom outside samplen 4
        set style line 1 lc rgb '#FF0000' lt 1 dashtype '.-' lw 6 pt 0
        set style line 2 lc rgb '#0000FF' lt 3 dashtype '.' lw 6 pt 0
        set style line 3 lc rgb '#00FF00' lt 3 dashtype 1 lw 6 pt 0

        #set fit quiet

        #tamanho dos pontos
        set pointsize 1.4

        plot '${DATAFILE_A}' every 1 using 1:2 title 'LTE' with linespoints ls 1,\
        '${DATAFILE_B}' every 1 u 1:2 t 'WiFi' with linespoints ls 2,\
        '${DATAFILE_B}' every 1 u 1:3 t 'Total' with linespoints ls 3
    " | gnuplot
}
# Two Y column and thwo X column
gnuYX(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    fi
    echo "
        set termoption dashed
        set encoding utf8
        set style data lines
        set grid
        set size 2,1

        set xlabel '${xLABEL}'
        set ylabel '${yLABEL}'
        set xrange ${xRANGE}
        set yrange ${yRANGE}

        set terminal postscript eps enhanced color font 'Helvetica,32'

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        set title '${gLABEL}'

        #table name below graph(naming curve by colour)
        set key right bottom outside samplen 4
        #estilo das linhas
        set style line 1 lc rgb '#241CA7' lt 1 dashtype 1 lw 3
        #set fit quiet

        #tamanho dos pontos
        set pointsize 1.8

        plot '${PLOTFILE}' using 1:2 t 'Lenght' with steps ls 1,\
             '${PLOTFILE}' using 1:2 with fillsteps fs solid 0.2 noborder ls 1 notitle
    " | gnuplot
}
gnuDotYX(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    fi
    echo "
        set termoption dashed
        set encoding utf8
        set style data lines
        set grid
        set size 2,1

        set xlabel '${xLABEL}'
        set ylabel '${yLABEL}'
        set xrange ${xRANGE}
        set yrange ${yRANGE}

        set terminal postscript eps enhanced color font 'Helvetica,32'

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        set title '${gLABEL}'

        #table name below graph(naming curve by colour)
        set key right bottom outside samplen 4
        #estilo das linhas
        set style line 1 lc rgb '#FF0000' lt 2 dashtype 4 pt 1 lw 4
        set style line 2 lc rgb '#FF0000' lt 1 dashtype 1 pt 5 lw 1

        set style line 3 lc rgb '#0000FF' lt 1 dashtype 1 pt 9 lw 2
        set style line 4 lc rgb '#0000FF' lt 1 dashtype 1 pt 9 lw 1

        #set fit quiet

        #tamanho dos pontos
        set pointsize 1

        plot  '${PLOTFILE}' using 1:2 t 'LTE' with linespoints ls 1,\
              '${PLOTFILE}' using 3:4 t 'WiFi' with linespoints ls 3
    " | gnuplot
}
# Two Y column and thwo X column
gnuSolidY2X(){

    if [ "${1}" != "" ]; then
        PLOTFILE=$1
    fi
    echo "
        set termoption dashed
        set encoding utf8
        set style data lines
        set grid
        set size 2,1

        set xlabel '${xLABEL}'
        set ylabel '${yLABEL}'
        set xrange ${xRANGE}
        set yrange ${yRANGE}

        set terminal postscript eps enhanced color font 'Helvetica,32'

        set output '| epstopdf --filter > ${PLOTFILE}.pdf'
        set title '${gLABEL}'

        #table name below graph(naming curve by colour)
        set key right bottom outside samplen 4
        #estilo das linhas
        set style line 1 lc rgb '#999999' lt 1 dashtype 2 lw 2
        set style line 2 lc rgb '#730099' lt 1 dashtype 1 pt 5 lw 3
        set style line 3 lc rgb '#00e673' lt 1 dashtype 1 lw 1
        set style line 4 lc rgb '#FF0000' lt 1 dashtype 1 pt 1 lw 3
        #set fit quiet

        #tamanho dos pontos
        set pointsize 0.7

        plot '${PLOTFILE}' using 3:2 with fillsteps fs solid 0.2 noborder ls 3 notitle,\
              '${PLOTFILE}' using 1:2 with points ls 2 notitle,\
              '${PLOTFILE}' using 3:2 with points ls 4 notitle
    " | gnuplot
}
plotCWND(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    count=`find ./ -maxdepth 1 -name "mptcp-CWND*" | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate Plot CWND.:.${NOCOLOR}"
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-CWND-1-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-CWND-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_B}

      MAX=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${DATAFILE_A}`
      MAXb=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${DATAFILE_B}`

      if [ ${MAXb} -gt ${MAX} ]; then
        MAX=${MAXb}
      fi
      yRANGE="[0:50]"
      xRANGE="[0:${simTime}]"

      gLABEL="Janela de Congestionamento (CWND) - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="Janela(pacotes)"

      #Chama funcao plot
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-mptcp-CWND.plot"
      gnuYYXX
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
      cp $DATAFILE_B "b.$PLOTFILE"

    fi
}

plotRTT(){

    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    count=`find ./ -maxdepth 1 -name "mptcp-RTT*" | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate Plot RTT.:.${NOCOLOR}"
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-RTT-1-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-RTT-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_B}

      MAX=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${DATAFILE_A}`
      MAXb=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${DATAFILE_B}`

      if [ ${MAXb} -gt ${MAX} ]; then
        MAX=${MAXb}
      fi
      yRANGE="[0:250]"
      xRANGE="[0:${simTime}]"

      gLABEL="Round Trip Time (RTT) - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="RTT(ms)"

      #Chama funcao plot
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-mptcp-RTT.plot"
      gnuYYXX
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
      cp $DATAFILE_B "b.$PLOTFILE"
    fi
}
plotCoDelQueueLenght(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-CoDel-length.plot"

    count=`cat ${FILEPREFIX}-CoDel-length.tr | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate CoDel Lenght.:.${NOCOLOR}"
      gawk '{a[$1]+=$2}END{for(k in a)printf("%.3f %s\n", k,a[k])}'  ${FILEPREFIX}-CoDel-length.tr | sort -g > ${PLOTFILE}
      MAXy=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${PLOTFILE}`

      yRANGE="[0:${MAXy}]"
      xRANGE="[0:${simTime}]"
      gLABEL="CoDel Queue Lenght - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Time(s)"
      yLABEL="Queue(Bytes)"

      #Chama funcao plot
      gnuYX
    fi
}

plotCoDelDropPackets(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7


    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-${QUEUE}-drop.plot"

    count=`cat ${FILEPREFIX}-${QUEUE}-drop.tr | wc -l`

    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate ${QUEUE} Drops.:.${NOCOLOR}"

      cat ${FILEPREFIX}-${QUEUE}-drop.tr | grep 0.10.1 > flow1.bfs
      cat ${FILEPREFIX}-${QUEUE}-drop.tr | grep 0.7.0 > flow2.bfs


      gawk '{printf("%.1f\n",$1 )}' flow1.bfs | uniq -c | gawk '{print $2}' > xcol1.bfs
      gawk '{printf("%.1f\n",$1 )}' flow1.bfs | uniq -c | gawk '{print $1}' > ycol1.bfs

      gawk '{printf("%.1f\n",$1 )}' flow2.bfs | uniq -c | gawk '{print $2}' > xcol2.bfs
      gawk '{printf("%.1f\n",$1 )}' flow2.bfs | uniq -c | gawk '{print $1}' > ycol2.bfs

      paste xcol1.bfs ycol1.bfs xcol2.bfs ycol2.bfs > ${PLOTFILE}
      rm *.bfs

      MAXy=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${PLOTFILE}`

      yRANGE="[0:45]"
      xRANGE="[0:${simTime}]"
      gLABEL="${QUEUE} Queue Drops - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="Número de Pacotes"

      #Chama funcao plot
      gnuDotYX
    fi
}

plotCoDelDropState(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-${QUEUE}-drop-state.plot"

    count=`cat ${FILEPREFIX}-${QUEUE}-drop-state.tr | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate ${QUEUE} Drops.:.${NOCOLOR}"
      gawk '{printf("%.4f %.4f %.4f\n",$1,$2-$1,$2 )}' ${FILEPREFIX}-${QUEUE}-drop-state.tr  > ${PLOTFILE}
      #gawk '{printf("%d\n %.4f\n",0, $2 )}' ${FILEPREFIX}-CoDel-drop-state.tr  > ${DATAFILE_B}
      seq 0.0 0.1 60.0 > xcol.bfs
      #paste xcol.bfs ${DATAFILE_A} ${DATAFILE_B} > ${PLOTFILE}
      rm *.bfs

      MAXy=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%f", max)}' ${PLOTFILE}`

      yRANGE="[0:${MAXy}]"
      xRANGE="[0:${simTime}]"
      gLABEL="${QUEUE} Queue Drops - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="Número de Pacotes"

      #Chama funcao plot
      gnuSolidY2X
    fi
}
plotCoDelSoujorTime(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-${QUEUE}-soujorn.plot"

    count=`cat ${FILEPREFIX}-${QUEUE}-sojourn.tr | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate ${QUEUE} Sojourn Times.:.${NOCOLOR}"

      gawk '{printf("%f %.6f\n",$3,$1/1000000 )}'  ${FILEPREFIX}-${QUEUE}-sojourn.tr > ${PLOTFILE}

      MAXy=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%.6f", max)}' ${PLOTFILE}`
      MAXx=`gawk 'BEGIN {max = 0} {if ($1>max) max=$1} END {printf("%.2f", max)}' ${PLOTFILE}`

      yRANGE="[0:0.45]"
      xRANGE="[0:${simTime}]"
      gLABEL="Variação do Tempo de permanência na fila - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Ocorrências no intervalor de ${simTime}"
      yLABEL="Tempo de permanência na fila(s)"

      #Chama funcao plot
      gnuYX
    fi
}
plotCoDelSoujornFreq(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-${QUEUE}-soujorn-freq.plot"

    count=`cat ${FILEPREFIX}-${QUEUE}-sojourn.tr | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate ${QUEUE} Sojourn Frequency.:.${NOCOLOR}"
      gawk '{printf("%.6f\n",$1/1000000000 )}' ${FILEPREFIX}-${QUEUE}-sojourn.tr | sort | uniq -c | gawk '{print $2}' > xcol.bfs
      gawk '{printf("%.6f\n",$1/1000000000 )}' ${FILEPREFIX}-${QUEUE}-sojourn.tr | sort | uniq -c | gawk '{print $1}' > ycol.bfs
      paste xcol.bfs ycol.bfs > ${PLOTFILE}
      rm *.bfs

      MAXy=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${PLOTFILE}`
      MAXx=`gawk 'BEGIN {max = 0} {if ($1>max) max=$1} END {printf("%.6f", max)}' ${PLOTFILE}`

      yRANGE="[0:${MAXy}]"
      xRANGE="[0:${MAXx}]"
      gLABEL="Frequência dos Tempos de permananência"
      xLABEL="Tempo(s)"
      yLABEL="Número de ocorrências"

      #Chama funcao plot
      gnuYX
    fi
}

plotDropTailDrops(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    PLOTFILE=$PLOT
    PLOTFILE+="-${FILEPREFIX}-DropTail-drop.plot"

    count=`cat ${FILEPREFIX}-DropTail-drop.tr | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate DropTail Drops.:.${NOCOLOR}"
      gawk '{printf("%.0f\n",$1 )}' ${FILEPREFIX}-DropTail-drop.tr | uniq -c | gawk '{print $2}' > xcol.bfs
      gawk '{printf("%.0f\n",$1 )}' ${FILEPREFIX}-DropTail-drop.tr | uniq -c | gawk '{print $1}' > ycol.bfs
      paste xcol.bfs ycol.bfs > ${PLOTFILE}
      rm *.bfs

      MAX=`gawk 'BEGIN {max = 0} {if ($2>max) max=$2} END {printf("%d", max)}' ${PLOTFILE}`

      yRANGE="[0:${MAX}]"
      xRANGE="[0:${simTime}]"
      gLABEL="DropTail Queue Drops - [${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="Número de Pacotes"

      #Chama funcao plot
      gnuYX
    fi
}

plotTroughput(){
    CC=$1
    QUEUE=$2
    RATE=$3
    DELAY=$4
    BUFFER=$5
    QUEUE_SIZE=$6
    FILEPREFIX=$7

    count=`find ./ -maxdepth 1 -name "${FILEPREFIX}-throughput*.tr" | wc -l`
    if [ ${count} != 0 ]; then

      echo -e "${YELLOW}.:.Generate Throughput ${QUEUE}.:.${NOCOLOR}"
      gawk '{printf("%.1f %.3f\n",$1,$2)}' ${FILEPREFIX}-throughput-0.tr > ${DATAFILE_A}
      gawk '{printf("%.1f %.3f\n",$1,$2)}' ${FILEPREFIX}-throughput-1.tr > datab.bfs
      paste ${DATAFILE_A} datab.bfs > tmp.bfs

      gawk '{printf("%.3f\n",$2+$4)}' tmp.bfs > total.bfs
      paste datab.bfs total.bfs >  ${DATAFILE_B}

      Clean "bfs"

      MAX=`gawk 'BEGIN {max = 0} {if ($3>max) max=$3} END {printf("%.3f", max)}' ${DATAFILE_B}`
      yRANGE="[0:1.3]"
      xRANGE="[0:${simTime}]"

      gLABEL="Vazão total e por fluxo MPTCP - [${CC}-${QUEUE}-${RATE}-${DELAY}-${BUFFER}-${QUEUE_SIZE}]"
      xLABEL="Tempo(s)"
      yLABEL="Throughput(Mbps)"

      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-throughput.plot"
      gnuYXX
      #BACKUP Plots
      cp $DATAFILE_A "a-$PLOTFILE"
      cp $DATAFILE_B "b-$PLOTFILE"

    fi
}
#Pacotes encaminhados por tempo de simulacao
# gawk '{printf("%.1f\n",$2 )}' mptcp-pkt-snd-tracer.tr | uniq -c
Plot(){
  #plotRTT Linked_Increases DropTail 0.5 0.300 65536 10000 bfs_Linked_Increases_DropTail_0.5_0.300_65536_10000_-0
  #plotRTT Linked_Increases CoDel 0.5 0.300 65536 10000 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10000_-0
  #plotCWND Linked_Increases DropTail 0.5 0.300 65536 10000 bfs_Linked_Increases_DropTail_0.5_0.300_65536_10000_-0
  #plotCWND Linked_Increases CoDel 0.5 0.300 65536 10000 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10000_-0
  #plotCWND2
  #plotTroughput Linked_Increases CoDel 0.5 0.300 65536 10 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10_-0
  #plotTroughput Linked_Increases DropTail 0.5 0.300 65536 10 bfs_Linked_Increases_DropTail_0.5_0.300_65536_10_-0
  #plotCoDelQueueLenght Linked_Increases CoDel 0.5 0.300 65536 10 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10_-0
  plotCoDelDropPackets RTT_Compensator CoDel 1 0.100 65536 1000 bfs_RTT_Compensator_CoDel_1.0_0.001_65536_1000-0
  #plotCoDelDropState RTT_Compensator CoDel 1 0.100 65536 1000 bfs_RTT_Compensator_CoDel_1.0_0.001_65536_1000_-0
  #plotCoDelSoujorTime Linked_Increases CoDel 0.5 0.300 65536 10 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10_-0
  #plotCoDelSoujornFreq Linked_Increases CoDel 0.5 0.300 65536 10 bfs_Linked_Increases_CoDel_0.5_0.300_65536_10_-0
  #plotDropTailDrops Linked_Increases DropTail 0.5 0.300 65536 10 bfs_Linked_Increases_DropTail_0.5_0.300_65536_10_-0
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
    echo "   -p|--plot                    Plot gnuplot graph from data set file"
    echo "   -h|--help                    Show this help message"
    echo "Examples:"
    echo "    ./run-p1.sh -r"
    echo "    ./run-p1.sh -r trace_"
    echo "    ./run-p1.sh -c"
    echo "    ./run-p1.sh -c xml"
    echo -e "${WHITE}\n"
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
        '-p'|'--plot' )
            Plot
        ;;

        *)
            ShowUsage
            exit 1
        ;;
    esac

    exit 0
}

main "$@"
