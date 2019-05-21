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

declare -i numRep=1 #35
declare -i simTime=120
declare -i logging=1
declare -i interference=1 #deixar rodar sem interferencia depois
declare -i pcap=1
declare -i MSS=


# Default data file name
declare FAKESIM="n"
declare PLOTGRAPHS="n"
declare DATAFILE="bfs"
declare DATAFILE_A="da_.dat"
declare DATAFILE_B="db_.dat"
declare PLOT="plot"
declare PLOTFILE
declare BACKUP_PLOTS="plots"
declare xLABEL=""
declare yLABEL=""

declare xRANGE=""
declare yRANGE=""
# Path of ns3 directory for running waf
declare NS3_PATH=~/git/ns3-mpaqm/
#declare NS3_PATH=/Volumes/vms/ns3-mpaqm/
Run(){

    # Create data file, optionally specified by user
    if [ "${1}" != "" ]; then
        DATAFILE=$1
    else
        echo -e "Prefix of trace file is ${DATAFILE}"
    fi

    # Taxas de transferencia 0.5 -- 100Mbps
    declare -a dataRate=(  "0.5"
                            "1"
                           #"2"
                        )
    # Delay Range 1-300
    declare -a delayRate=(  "0.001"
                            "0.010"
                            "0.100"
                            "0.300"
                            "0.500"
                            "1.000"
                            )
    # Queue Size 1-1000
    declare -a queueSize=(
                            #"100"
                            "100"
                            )
    # Queue Size 1-1000
    declare -a bufferSize=(
                              "65536"
                            )
    declare -a path2use=(
                          #"0" #MPTCP
                          "1" # WIFI only
                          "2" # LTE only
    )
    # Queue Type
    declare -a queueType=(  "DropTail"
                            "CoDelQueue"
                            "CoDelQueueLifo"
                            #"Fq_CoDel"
                            )
    # Congestion Control
    declare -a congestionControl=(
                            "RTT_Compensator"
                            "Linked_Increases"
                            "Uncoupled_TCPs"
                            )

    echo -e "${BLUE}Running DropTail-Codel Simluation... ${GRAY}\n"
    c=0
    # Run each combination of settings
for (( path=0; path<${#path2use[@]}; ++path ))
do
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
                fileNamePrefix+=${path2use[$path]}"_"
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
                    echo -e "${colors[c++]} - Run \"globecom-one-receiver\" ${NAME}"

                    #IF FAKESIM, RUN IN DIRECTORY OF BACKUP TRACES
                    if [ "${FAKESIM}" != "y" ]; then
                        #fileNamePrefix+=${i}
                        ./waf --run "scratch/globecom-one-receiver \
                              --path2use=${path2use[$path]} \
                              --congestionControl=${congestionControl[$congestion]} \
                              --queueType=${queueType[$qtype]} \
                              --dataRate=${dataRate[$rate]} \
                              --delayRate=${delayRate[$delay]} \
                              --bufferSize=${bufferSize[$bsize]} \
                              --queueSize=${queueSize[$qsize]} \
                              --simTime=${simTime} \
                              --logging=${logging} \
                              --interference=${interference} \
                              --pcap=${pcap} \
                              --repRun=${i}\
                              --fileNamePrefix=${fileNamePrefix}-${i}"  > output-${i}.dat 2>&1

                              #Rename Traces
                              RenameTraces $NAME
                    fi
                    PARAMS="${congestionControl[$congestion]} ${queueType[$qtype]} ${dataRate[$rate]} ${delayRate[$delay]} ${bufferSize[$bsize]} ${queueSize[$qsize]} ${NAME}"
                    if [ "${path2use}" != "0" ]; then
                        dat-1RTT ${NAME}
                        dat-1CWND ${NAME}
                        dat-1Troughput ${NAME}
                    else
                      datRTT ${NAME}
                      datCWND ${NAME}
                      datTroughput ${NAME}
                    fi


                    if ((c > ${#colors[@]}))
                    then
                        c=1
                    fi

                    if [ "${FAKESIM}" != "y" ]; then
                      echo -e "${RED} Moving Repetitions ${NOCOLOR}"
                      MoveRep ${NAME} ${fileNamePrefix} ${queueType[$qtype]} ${path2use[$path]}
                      Clean
                      Clean "pdf"
                      Clean "plot"
                    fi
                done
                echo -e "${NOCOLOR}"
            done
          done
        done
      done
    done
  done #congestion
done #path

    echo -e "${RED} Finished simulation! ${NOCOLOR}\n"
}
RenameTraces(){
  # Move trace files with prefixName to folder traces
  for file in *tracer.tr
  do
      mv "$file" "${file/tracer/${1}-}"
  done
}
MoveRep(){
  NAME=$1
  FILEPREFIX=$2
  NPATH=$3
  #RTT -- NR is number rowns inner GAWK
  gawk '{s+=$2} END {print s/NR}' a.plot-${NAME}-RTT.plot > rtt.a.bfs
  echo "" > rtt.b.bfs
  if [ "${NPATH}" == "0" ]; then
    gawk '{s+=$2} END {print s/NR}' b.plot-${NAME}-RTT.plot > rtt.b.bfs
  fi
  paste rtt.a.bfs rtt.b.bfs >> reps/rep.${FILEPREFIX}.RTT
  Clean "bfs"

  #CWND
  gawk '{s+=$2} END {print s/NR}' a.plot-${NAME}-CWND.plot > cwnd.a.bfs
  echo "" > cwnd.b.bfs
  if [ "${NPATH}" == "0" ]; then
    gawk '{s+=$2} END {print s/NR}' b.plot-${NAME}-CWND.plot > cwnd.b.bfs
  fi
  paste cwnd.a.bfs cwnd.b.bfs >> reps/rep.${FILEPREFIX}.CWND
  Clean "bfs"

  #THROUGHPUT - LTE - WIFI
  gawk '{s+=$2} END {print s/NR}' a.plot-${NAME}-throughput.plot > throughput.a.bfs
  echo "" > throughput.b.bfs
  if [ "${NPATH}" == "0" ]; then
    gawk '{s+=$2} END {print s/NR}' b.plot-${NAME}-throughput.plot > throughput.b.bfs
  fi
  paste throughput.a.bfs throughput.b.bfs >> reps/rep.${FILEPREFIX}.THR
  Clean "bfs"

  #GoodPut - LTE - WIFI - UDP
  gawk '/./{line=$0} END{print line}' ${NAME}-goodput.xr > goodput.bfs
  cat goodput.bfs >> reps/rep.${FILEPREFIX}.GDP
  Clean "bfs"

  if [ "${queueType[$qtype]}" != "DropTail" ]; then

    #QUEUE LENGHT
    FILE="lenght.bfs"
    count=`cat ${NAME}-length.tr | wc -l`
    if [ ${count} != 0 ]; then
      # Save file to 1 column
      gawk '{printf("%.4f\n",$2)}' ${NAME}-length.tr > ${FILE}
      echo $(Add $FILE) >> reps/rep.${FILEPREFIX}.LGHT
      Clean "bfs"
    fi

    #QUEUE SOJOURN
    FILE="sojourn.bfs"
    count=`cat ${NAME}-sojourn.tr | wc -l`
    if [ ${count} != 0 ]; then

      gawk '{printf("%.6f\n",$1/1000000 )}' ${NAME}-sojourn.tr > ${FILE}
      echo $(Add $FILE) >> reps/rep.${FILEPREFIX}.SJN
      Clean "bfs"
    fi

    #QUEUE DROPS
    FILE="drops.bfs"
    count=`cat ${NAME}-drop.tr | wc -l`
    if [ ${count} != 0 ]; then
      gawk '/./{line=$0} END{print $2,$1}' ${NAME}-drop.tr > ${FILE}
      cat $FILE >> reps/rep.${FILEPREFIX}.DRP
      Clean "bfs"
    else
      echo $count >> reps/rep.${FILEPREFIX}.DRP
    fi

  fi

  if [ "${queueType[$qtype]}" == "DropTail" ]; then
    #DROPs
    count=`cat ${NAME}-drop.tr | wc -l`
    echo $count >> reps/rep.${FILEPREFIX}.DRP
  fi


}
Add(){
  FILE=$1

  MED=$(CalculaMedia ${FILE})
  DVP=$(CalculaDesvioPadrao ${FILE})
  ER=$(CalculaErro ${FILE})

  echo "$MED $DVP $ER"
}
# DATA FROM File must BE IN column 1
CalculaMedia(){

  if [ "${1}" == "" ]; then
   echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=`cat ${FILEP} | gawk '{s+=$1;}END{printf "%f",s/NR;}'`
  echo $mean
}
CalculaDesvioPadrao(){

  if [ "${1}" == "" ]; then
    echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=$(CalculaMedia ${FILEP})
  std=`gawk -v med=$mean '{aux=$1-med; stDev+=aux*aux;}END{ printf "%f", sqrt(stDev/(NR-1));}' ${FILEP}`

  echo $std
}
CalculaErro(){

  if [ "${1}" == "" ]; then
    echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  #Number of rows
  N=`gawk 'END{printf("%d",NR)}' ${FILEP}`
  if [ ${N} -gt 30 ]; then
    Z=1.96
    mean=$(CalculaMedia ${FILEP})
    std=$(CalculaDesvioPadrao ${FILEP})
    errop=`gawk -v stdev=$std -v z=$Z 'END{printf("%f",(z*stdev)/sqrt(NR))}' ${FILEP}`
  fi
  echo "$errop"
}

Clean(){

    cd ${NS3_PATH}

    if [ "$1" == "" ]; then
        Clean "tmp"
        Clean "dat"
        Clean "pcap"
        Clean "plt"
        Clean "tr"
        Clean "xr"
    else
        count=`find ./ -maxdepth 1 -name "*.${1}" | wc -l`
        if [ ${count} != 0 ]; then
            rm *.${1}
            echo "Removed ${count} .${1} files"
        fi
    fi

}


datCWND(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "mptcp-CWND*" | wc -l`
    if [ ${count} != 0 ]; then
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-CWND-1-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-CWND-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_B}
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-CWND.plot"
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
      cp $DATAFILE_B "b.$PLOTFILE"
    fi
}
datRTT(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "mptcp-RTT*" | wc -l`
    if [ ${count} != 0 ]; then
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-RTT-1-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-RTT-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_B}
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-RTT.plot"
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
      cp $DATAFILE_B "b.$PLOTFILE"
    fi
}
dat-1CWND(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "mptcp-CWND*" | wc -l`
    echo $count

    if [ ${count} != 0 ]; then
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-CWND-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-CWND.plot"
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
    fi
}
dat-1RTT(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "mptcp-RTT*" | wc -l`
    if [ ${count} != 0 ]; then
      gawk '{printf("%.2f %d\n",$1,$2 )}' mptcp-RTT-0-${FILEPREFIX}-.tr | awk '!a[$1]++' > ${DATAFILE_A}
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-RTT.plot"
      #BACKUP Plots
      cp $DATAFILE_A "a.$PLOTFILE"
    fi
}

datTroughput(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "${FILEPREFIX}-throughput*.tr" | wc -l`
    if [ ${count} != 0 ]; then
      gawk '{printf("%.1f %.3f\n",$1,$2)}' ${FILEPREFIX}-throughput-0.tr > ${DATAFILE_A}
      gawk '{printf("%.1f %.3f\n",$1,$2)}' ${FILEPREFIX}-throughput-1.tr > datab.bfs
      paste ${DATAFILE_A} datab.bfs > tmp.bfs
      gawk '{printf("%.3f\n",$2+$4)}' tmp.bfs > total.bfs
      paste datab.bfs total.bfs >  ${DATAFILE_B}
      Clean "bfs"
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-throughput.plot"
      cp $DATAFILE_A "a.$PLOTFILE"
      cp $DATAFILE_B "b.$PLOTFILE"
    fi
}

dat-1Troughput(){
    FILEPREFIX=$1
    count=`find ./ -maxdepth 1 -name "${FILEPREFIX}-throughput*.tr" | wc -l`
    if [ ${count} != 0 ]; then
      gawk '{printf("%.1f %.3f\n",$1,$2)}' ${FILEPREFIX}-throughput-0.tr > ${DATAFILE_A}
      Clean "bfs"
      PLOTFILE=$PLOT
      PLOTFILE+="-${FILEPREFIX}-throughput.plot"
      cp $DATAFILE_A "a.$PLOTFILE"
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

        *)
            ShowUsage
            exit 1
        ;;
    esac

    exit 0
}

main "$@"
