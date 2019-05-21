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

declare -i numRep=35
declare -i simTime=120
declare -i logging=1
declare -i interference=1
declare -i pcap=1
declare -i MSS=1458


# Default data file name
declare FAKESIM="n"

# Default plot variables
declare PLOTGRAPHS="n"
declare DATAFILE="bfs"
declare DATAFILE_A="da_.dat"
declare DATAFILE_B="db_.dat"
declare PLOT="plot"
declare PLOTFILE
declare OUTPUT
declare -a DATAFILES
declare BACKUP_PLOTS="plots"
declare xLABEL=""
declare yLABEL=""
declare gTITLE=""
declare xRANGE=""
declare yRANGE=""
declare yTICS=""
declare xTICS=""
# Path of ns3 directory for running waf
#declare NS3_PATH=/Volumes/vms/
declare NS3_PATH=~/git/ns3-mpaqm/

# Taxas de transferencia 0.5 -- 100Mbps
declare -a dataRate=(  "1"
                       #"2"
                    )
# Delay Range 1-300
declare -a delayRate=(  "0.001"
                        "0.010"
                        "0.100"
                        "0.300"
                        #"0.500"
                        #"1.000"
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
# Queue Type
declare -a queueType=(
                        "CoDelQueue"
                        "CoDelQueueLifo"
                        #"DropTail"
                        #"Fq_CoDel"
                        )
# Queue Type
declare -a queueName=(  "CoDel-FIFO"
                        "CoDel-LIFO"
                        #"DropTail"
                        #"FQ-LIFO"
                        )
# Congestion Control
declare -a congestionControl=(
                        "RTT_Compensator"
                        "Linked_Increases"
                        "Uncoupled_TCPs"
                        )

declare -a suphix=(
                        "CWND"
                        "DRP"
                        "GDP"
                        "RTT"
                        "THR"
                        "SJN"
                        "LGHT"
                        )

Run(){

    # Create data file, optionally specified by user
    if [ "${1}" != "" ]; then
        DATAFILE=$1
    else
        echo -e "Prefix of trace file is ${DATAFILE}"
    fi

    echo -e "${BLUE}Running DropTail-Codel Simluation... ${GRAY}\n"
    c=0
    # Run each combination of settings
    for (( congestion=0; congestion<${#congestionControl[@]}; ++congestion ))
    do

      for (( suf=0; suf<${#suphix[@]}; ++suf ))
      do


      for (( qtype=0; qtype<${#queueType[@]}; ++qtype ))
      do

        PLTFILE="plots/plt.${congestionControl[$congestion]}.${queueType[$qtype]}.${suphix[$suf]}.plot"
        if [ "${suphix[$suf]}" == "CWND" ]; then
          touch $PLTFILE
        fi
      for (( delay=0; delay<${#delayRate[@]}; ++delay ))
      do

                F1="f1.bfs"
                F2="f2.bfs"
                F3="f3.bfs"
                fileNamePrefix=${DATAFILE}"_"
                fileNamePrefix+=${congestionControl[$congestion]}"_"
                fileNamePrefix+=${queueType[$qtype]}"_"
                fileNamePrefix+=${dataRate[$rate]}"_"
                fileNamePrefix+=${delayRate[$delay]}"_"
                fileNamePrefix+=${bufferSize[$bsize]}"_"
                fileNamePrefix+=${queueSize[$qsize]}

                NAME="rep.${fileNamePrefix}.${suphix[$suf]}"

                count=`find reps/ -maxdepth 1 -name "${NAME}" | wc -l`
                if [ ${count} != 0 ]; then
                  echo -e "${YELLOW} ${NAME} ${NOCOLOR}"
                  if [ "${suphix[$suf]}" == "CWND" ]; then
                      gawk '{printf(" %.1f \n", $1)}' reps/${NAME} > ${F1}
                      gawk '{printf(" %.1f \n", $2)}' reps/${NAME} > ${F2}

                      echo $(Add $F1) >> t1.bfs
                      echo $(Add $F2) >> t2.bfs
                      paste t1.bfs t2.bfs >> $PLTFILE
                      #echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "RTT" ]; then
                      gawk '{printf(" %.2f \n", $1)}' reps/${NAME} > ${F1}
                      gawk '{printf(" %.2f \n", $2)}' reps/${NAME} > ${F2}

                      echo $(Add $F1) >> t1.bfs
                      echo $(Add $F2) >> t2.bfs
                      paste t1.bfs t2.bfs >> $PLTFILE
                      #echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "GDP" ]; then
                      gawk '{printf(" %f \n", ($1+$2))}' reps/${NAME} > ${F1}
                      #gawk '{printf(" %.2f \n", $2)}' reps/${NAME} > ${F2}
                      #gawk '{printf(" %.2f \n", $3)}' reps/${NAME} > ${F3}
                      #echo $(Add $F1) >> t1.bfs
                      #echo $(Add $F2) >> t2.bfs
                      #echo $(Add $F3) >> t3.bfs
                      #paste t1.bfs t2.bfs t3.bfs >> $PLTFILE
                      echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "THR" ]; then
                      gawk '{printf(" %f \n", ($1+$2))}' reps/${NAME} > ${F1}
                      #gawk '{printf(" %.2f \n", $2)}' reps/${NAME} > ${F2}
                      #gawk '{printf(" %.2f \n", $3)}' reps/${NAME} > ${F3}
                      #echo $(Add $F1) >> t1.bfs
                      #echo $(Add $F2) >> t2.bfs
                      #echo $(Add $F3) >> t3.bfs
                      #paste t1.bfs t2.bfs t3.bfs >> $PLTFILE
                      echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "DRP" ]; then
                      if [ "${queueType[$qtype]}" == "DropTail" ]; then
                        gawk '{printf(" %d \n", ($1))}' reps/${NAME} > ${F1}
                      fi
                      if [ "${queueType[$qtype]}" == "CoDelQueue" ]; then
                        gawk '{printf(" %d \n", ($2))}' reps/${NAME} > ${F1}
                      fi
                      if [ "${queueType[$qtype]}" == "CoDelQueueLifo" ]; then
                        gawk '{printf(" %d \n", ($1))}' reps/${NAME} > ${F1}
                      fi
                      echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "LGHT" ]; then
                      gawk '{printf(" %f \n", ($1))}' reps/${NAME} > ${F1}
                      #gawk '{printf(" %.2f \n", $2)}' reps/${NAME} > ${F2}
                      #gawk '{printf(" %.2f \n", $3)}' reps/${NAME} > ${F3}
                      #echo $(Add $F1) >> t1.bfs
                      #echo $(Add $F2) >> t2.bfs
                      #echo $(Add $F3) >> t3.bfs
                      #paste t1.bfs t2.bfs t3.bfs >> $PLTFILE
                      echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                  if [ "${suphix[$suf]}" == "SJN" ]; then
                      gawk '{printf(" %f \n", ($1))}' reps/${NAME} > ${F1}
                      #gawk '{printf(" %.2f \n", $2)}' reps/${NAME} > ${F2}
                      #gawk '{printf(" %.2f \n", $3)}' reps/${NAME} > ${F3}
                      #echo $(Add $F1) >> t1.bfs
                      #echo $(Add $F2) >> t2.bfs
                      #echo $(Add $F3) >> t3.bfs
                      #paste t1.bfs t2.bfs t3.bfs >> $PLTFILE
                      echo $(Add $F1) >> $PLTFILE
                      Clean "bfs"
                  fi
                fi
          done
      done

    done
  done
    echo -e "${RED} Finished PLOTS! ${NOCOLOR}\n"
}

PlotCWND(){
  CC=$1
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*CWND*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
    for (( fe=0; fe<${#DATAFILES[@]}; ++fe ))
    do
      sty=$(($fe + 1))
      echo -e "${BLUE} ${DATAFILES[$fe]} ${queueType[$fe]} ${NOCOLOR}\n"

      gTITLE="${queueName[$fe]} Congestion Window"
      xLABEL="Delay (ms)"
      yLABEL="Average Size(packet)"
      yRANGE="[0:40]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="10"
      PLOTFILE=${DATAFILES[$fe]}
      OUTPUT=`echo ${DATAFILES[$fe]} | cut -c9-`
      plotBox
    done
  fi
}
PlotMultiCWND(){
  CC=$1
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*CWND*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="Congestion Window"
      xLABEL="Delay (ms)"
      yLABEL="Average Size (packet)"
      yRANGE="[0:40]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="10"
      OUTPUT="$CC-CWND"
      plotMultiPlotBox
  fi
}
PlotRTT(){
  CC=$1
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*RTT*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
    for (( fe=0; fe<${#DATAFILES[@]}; ++fe ))
    do
      sty=$(($fe + 1))
      echo -e "${BLUE} ${DATAFILES[$fe]} ${queueType[$fe]} ${NOCOLOR}\n"

      gTITLE="${queueName[$fe]} Round Trip Time"
      xLABEL="Delay (ms)"
      yLABEL="Average RTT (ms)"
      yRANGE="[0:1250]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="300"
      PLOTFILE=${DATAFILES[$fe]}
      OUTPUT=`echo ${DATAFILES[$fe]} | cut -c9-`
      plotBox
    done
  fi
}
PlotMultiRTT(){
  CC=$1
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*RTT*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="Round Trip Time"
      xLABEL="Delay (ms)"
      yLABEL="Average RTT (ms)"
      yRANGE="[0:3000]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="300"
      OUTPUT="$CC-RTT"
      plotMultiPlotBox
  fi
}
PlotGDP(){
  CC=$1
  echo -e "PLOT GOODPUT GRAPHS"
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*GDP*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="${queueName[$fe]} Goodput"
      xLABEL="Delay (ms)"
      yLABEL="Average Goodput (Mbps)"
      yRANGE="[0:1]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="0.1"
      OUTPUT="$CC-GPD"
      plotBars
  fi
}
PlotTHR(){
  CC=$1
  echo -e "PLOT THROUGHPUT GRAPHS"
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*THR*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="${queueName[$fe]} Throughput"
      xLABEL="Delay (ms)"
      yLABEL="Average Throughput (Mbps)"
      yRANGE="[0:1]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="0.1"
      OUTPUT="$CC-THR"
      plotBars
  fi
}
PlotDRP(){
  CC=$1
  echo -e "PLOT DROPS GRAPHS"
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*DRP*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="${queueName[$fe]} Queue Drops"
      xLABEL="Delay (ms)"
      yLABEL="Average Drops (packet)"
      yRANGE="[-10:140]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="25"
      OUTPUT="$CC-DRP"
      plotBars
  fi
}
PlotLGHT(){
  CC=$1
  echo -e "PLOT LENGHT QUEUE GRAPHS"
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*LGHT*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="${queueName[$fe]} Queue Size"
      xLABEL="Delay (ms)"
      yLABEL="Average Size(packet)"
      yRANGE="[0:20]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="2"
      OUTPUT="$CC-LGHT"
      plotBars
  fi
}
PlotSJN(){
  CC=$1
  echo -e "PLOT LENGHT QUEUE GRAPHS"
  DATAFILES=(`find ./plots -maxdepth 1 -iname "*$CC*SJN*"`)
  if [ ${#DATAFILES[@]} != 0 ]; then
      gTITLE="${queueName[$fe]} Sojourn Time"
      xLABEL="Delay (ms)"
      yLABEL="Average Sojourn (ms)"
      yRANGE="[0:0.1]"
      xRANGE="[-0.5:3.5]"
      xTICS="('1' 0, '10' 1, '100' 2, '300' 3)"
      yTICS="0.01"
      OUTPUT="$CC-SJN"
      plotBars
  fi
}
Add(){
  FILE=$1
  MED=$(CalculaMedia ${FILE})
  #DVP=$(CalculaDesvioPadrao ${FILE})
  ER=$(CalculaErro ${FILE})
  # Aqui tinha um erro porque o BC nao imprime zero a esquerda do ponto
  LI=`echo "($MED - $ER)" | bc | gawk '{printf "%.3f", $0}'`
  LS=`echo "($MED + $ER)" | bc | gawk '{printf "%.3f", $0}'`
  #echo -e "$MED\\t$LI\\t$LS \\t$ER"
  echo -e "$MED $LI $LS"
  #echo -e "$MED"
}
CalculaMedia(){

  if [ "${1}" == "" ]; then
   echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=`cat ${FILEP} | gawk '{s+=$1;}END{printf "%.2f",s/NR;}'`
  echo $mean
}
CalculaDesvioPadrao(){
  if [ "${1}" == "" ]; then
    echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=$(CalculaMedia ${FILEP})
  std=`gawk -v med=$mean '{aux=$1-med; stDev+=aux*aux;}END{ printf "%.2f", sqrt(stDev/(NR-1));}' ${FILEP}`

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
  if [ ${N} -gt 4 ]; then
    Z=1.96
    mean=$(CalculaMedia ${FILEP})
    std=$(CalculaDesvioPadrao ${FILEP})
    errop=`gawk -v stdev=$std -v z=$Z 'END{printf("%.3f",(z*stdev)/sqrt(NR))}' ${FILEP}`
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
        if [ "$2" != "" ]; then
          count=`find ./$2 -maxdepth 1 -iname "*.${1}" | wc -l`
          if [ ${count} != 0 ]; then
              rm $2/*.${1}
              echo "Removed ${count} .${1} files"
          fi
        else
          count=`find ./ -maxdepth 1 -name "*.${1}" | wc -l`
          if [ ${count} != 0 ]; then
              rm *.${1}
              echo "Removed ${count} .${1} files"
          fi
        fi
    fi

}
plotBox(){

  echo "
          ########################### BEGIN ####################################
          reset
          #formato de codificação
          set termoption dashed
          set encoding utf8

          #formato do arquivo, estilo da fonte e tamanho da fonte
          set terminal postscript eps enhanced  color 'Helvetica, 28'
          #set terminal postscript eps enhanced color font 'Helvetica,22'

          #arquivo de saída em formato pdf
          set output '| epstopdf --filter > pdfs/${OUTPUT}.pdf'

          #cria estilo para a caixa e os pontos
          set style line 1 lc rgb '#FF0000' lt 1 dashtype 1 lw 2 pt 3 pi -1
          set style line 2 lc rgb '#0000FF' lt -1 dashtype 1 lw 2 pt 7 pi -1
          set style line 3 lc rgb '#B5B5B5' lt 3 dashtype 2 lw 0.5
          set style line 4 lc rgb '#000000' lt 1 dashtype '-' lw 0.3
          set pointsize 1.8

          #titulos
          #set title '$gTITLE' #(título do gráfico)
          set xlabel '${xLABEL}'
          set ylabel '${yLABEL}'

          #posição da legenda
          set key left top samplen 4 font 'Helvetica,22' reverse Left


          #configura os valores dos eixos x e y
          set ytics ${yTICS} font 'Helvetica,22'
          set yrange ${yRANGE}
          set xtics  ${xTICS} font 'Helvetica,22'
          set xrange ${xRANGE}

          #cria a grade
          set grid ytics ls 4
          set grid xtics ls 4

          set pointintervalbox 3

          plot '${PLOTFILE}' using 0:4 with linespoints ls 2 notitle, \
               '${PLOTFILE}' using 0:1 with linespoints ls 1 notitle, \
               '${PLOTFILE}' using 0:1:2:3 t 'WiFi' with errorbars ls 1, \
               '${PLOTFILE}' using 0:4:5:6 t 'LTE' with errorbars ls 2, \



" | gnuplot
}
plotMonoBox(){

  echo "
          ########################### BEGIN ####################################
          reset
          #formato de codificação
          set termoption dashed
          set encoding utf8

          #formato do arquivo, estilo da fonte e tamanho da fonte
          set terminal postscript eps enhanced  color 'Helvetica, 28'
          #set terminal postscript eps enhanced color font 'Helvetica,22'

          #arquivo de saída em formato pdf
          set output '| epstopdf --filter > /Volumes/vms/${OUTPUT}.pdf'

          #cria estilo para a caixa e os pontos
          set style line 1 lc rgb '#FF0000' lt 1 dashtype 1 lw 1 pt 3
          set style line 2 lc rgb '#0000FF' lt 1 dashtype 1 lw 2 pt 6
          set style line 3 lc rgb '#B5B5B5' lt 3 dashtype 1 lw 0.5 pt 7
          set style line 4 lc rgb '#000000' lt 1 dashtype '-' lw 0.3

          set pointsize 1.8

          #titulos
          #set title '$gTITLE' #(título do gráfico)
          set xlabel '${xLABEL}'
          set ylabel '${yLABEL}'

          #posição da legenda
          set key left top samplen 4 font 'Helvetica,22' reverse Left

          #configura os valores dos eixos x e y
          set ytics ${yTICS} font 'Helvetica,18'
          set yrange ${yRANGE}
          set xrange ${xRANGE}
          set xtics nomirror scale 0 font 'Helvetica,18'
          #cria a grade
          set grid ytics ls 4
          set grid xtics ls 4

          plot '${PLOTFILE}' with points notitle ls 2,\
               '${PLOTFILE}' with lines notitle ls 3,\
               '${PLOTFILE}' using 1:2:3:4:xticlabels(1) t 'Theta' with yerrorbars ls 2

" | gnuplot
}
plotMultiPlotBox(){

  gnucmd=`echo "
          ########################### BEGIN ####################################
          reset
          #formato de codificação
          set termoption dashed
          set encoding utf8

          #formato do arquivo, estilo da fonte e tamanho da fonte
          set terminal postscript eps enhanced color 'Helvetica, 28'
          #set terminal postscript eps enhanced color font 'Helvetica,22'

          #arquivo de saída em formato pdf
          set output '| epstopdf --filter > pdfs/${OUTPUT}.pdf'

          #set lmargin at screen 0.15
          #set rmargin at screen 0.95

          #cria estilo para a caixa e os pontos
          set style line 1 lc rgb '#FF0000' lt 1 dashtype 1 lw 1 pt 3
          set style line 2 lc rgb '#0000FF' lt 1 dashtype 1 lw 1 pt 7
          set style line 3 lc rgb '#B5B5B5' lt 3 dashtype 2 lw 0.5
          set style line 4 lc rgb '#000000' lt 1 dashtype '-' lw 0.3
          #set style line 4 lc rgb '#B5B5B5' lt 3 lw 1

          #Tamanho do grafico
          set size 1,1
          set pointsize 1.2

          #titulos

          set xlabel '${xLABEL}'
          set ylabel '${yLABEL}' offset 1

          #posição da legenda
          set key left top samplen 4 reverse Left

          #configura os valores dos eixos x e y
          set ytics ${yTICS} font 'Helvetica,14'
          set yrange ${yRANGE}
          set xtics  ${xTICS}
          set xrange ${xRANGE}

          #cria a grade
          set grid ytics ls 4
          set grid xtics ls 4

          #set origin 0,0
          set multiplot layout 2,2 \
              margins 0.08,0.98,0.08,0.94 \
              spacing 0.10,0.18
"`

for (( fe=0; fe<${#DATAFILES[@]}; ++fe ))
do
  gnucmd+=`echo -e "\n
  #set title '${queueName[$fe]} - $gTITLE '
  plot '${DATAFILES[$fe]}' using 0:1:2:3 t 'WiFi' with errorbars ls 1, \
            '${DATAFILES[$fe]}' using 0:4:5:6 t 'LTE' with errorbars ls 2 "`

done
gnucmd+=`echo -e "\n unset multiplot"`
#echo -e "$gnucmd"
echo -e "$gnucmd" | gnuplot
}
plotBars(){

  gnucmd=`echo "
          ########################### BEGIN ############################
          reset
          #formato de codificação
          set encoding utf8
          set terminal postscript eps enhanced monochrome font 'Helvetica,28'
          set output '| epstopdf --filter > pdfs/${OUTPUT}.pdf'


          #configuração do tipo de gráfico
          set style data histogram
          set style histogram cluster gap 2
          set style fill pattern border -1
          set boxwidth 0.95

          #intervalo de confiança
          set style histogram errorbars lw 1

          #Tamanho do grafico
          set size 1.0,1.0

          #posição da legenda
          set key left top samplen 4 font 'Helvetica,22' reverse Left

          #configura os valores dos eixos x e y
          set ytics ${yTICS} font 'Helvetica,22'
          set yrange ${yRANGE}
          set xtics  ${xTICS} font 'Helvetica,22'
          set xrange ${xRANGE}

          #configuração de estilo para a grade
          set style line 1 lt 1 dashtype 1 lw 1 pt 7
          set style line 2 lt 1 dashtype 1 lw 1 pt 7
          set style line 3 lt 1 dashtype 1 lw 1 pt 7
          set style line 4 lt 1 dashtype 1 lw 1
          set style line 5 lt 3 dashtype 2 lw 0.5

          #cria a grade
          set grid ytics ls 5

          #titulos
          #set title '$gTITLE'
          set xlabel '${xLABEL}'
          set ylabel '${yLABEL}'
        " `
  gnucmd+="plot "
  for (( fe=0; fe<${#DATAFILES[@]}; ++fe ))
  do
    sty=$(($fe + 1))
    gnucmd+="'${DATAFILES[$fe]}' using 1:2:3 t '${queueName[$fe]}' ls $sty, "
  done
  #echo -e "$gnucmd"
  echo -e "$gnucmd" | gnuplot

}

Plot(){
  echo -e "Plots ${#congestionControl[@]}"
  # Run each combination of settings
  for (( congestion=0; congestion<${#congestionControl[@]}; ++congestion ))
  do
      echo -e "${GRAY} Plot ${congestionControl[$congestion]} ${NOCOLOR}\n"
      PlotCWND ${congestionControl[$congestion]}
      #PlotMultiCWND ${congestionControl[$congestion]}
      #PlotMultiRTT ${congestionControl[$congestion]}
      PlotRTT ${congestionControl[$congestion]}
      PlotGDP ${congestionControl[$congestion]}
      PlotTHR ${congestionControl[$congestion]}
      PlotDRP ${congestionControl[$congestion]}
      PlotLGHT ${congestionControl[$congestion]}
      PlotSJN ${congestionControl[$congestion]}

  done
  #PlotTheta $1
  echo "END PLOT"
}
PlotTheta(){
  PLOTFILE=$1
  echo -e "${BLUE} Theta ${NOCOLOR}\n"
  gTITLE="Average Value of Theta"
  xLABEL="Runs"
  yLABEL="Mean value"
  yRANGE="[0:5]"
  xRANGE="[1:32]"
  yTICS="0.5"
  OUTPUT="theta"
  plotMonoBox

}
Theta(){
  MAXV="1000"
  REP="31"
  vFILE="values.kx"
  plotFILE="plot.theta.px"
  tFILE="teta.kx"

  touch $vFILE
  touch $tFILE

  Clean "px"
  echo -e "BEGIN TEST\n"
  for (( rep=1; rep<=$REP; ++rep ))
  do
  printf "REP $rep: \t"
  for (( ct=0; ct<$MAXV; ++ct ))
  do
    VALUE=`ruby -e '1.upto(1){ print "%.0f\n" % (rand * 15) }'`
    echo -e "$VALUE" >> $vFILE
    MEAN=`gawk '{s+=$1;}END{printf "%.0f",s/NR;}' ${vFILE}`
    MAX=`gawk 'BEGIN {max = 0} {if ($1>max) max=$1} END {printf("%.0f", max)}' $vFILE `
    MIN=`gawk 'BEGIN {max = 1} {if ($1<max) max=$1} END {printf("%.0f", max)}' $vFILE `
    if [ ${MEAN} != 0 ]; then
      THETA=`echo "( 1+($MAX)/($MEAN) )" | bc | gawk '{printf("%.0f", $1)}'`
      echo -e "$THETA $VALUE $MEAN $MIN $MAX" >> $tFILE
    fi
  done
  #cat $tFILE
  echo "$rep $(Add $tFILE)" >> $plotFILE
  rm *.kx
  done
  echo "END TEST"
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
        '-t'|'--theta' )
            Theta $2
        ;;
        '-dc'|'--clean' )
            Clean $2 $3
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
