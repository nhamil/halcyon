echo "Params 1"
echo $3
echo "Params 2"
echo $4

cutechess-cli \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 initstr="settune $3" \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 initstr="settune $4" \
    -each proto=uci tc=1+0.08 timemargin=9999 book=./varied.bin \
    -pgnout ./tune-sprt.pgn \
    -bookmode ram \
    -sprt elo0=0 elo1=5 alpha=0.01 beta=0.01 \
    -ratinginterval 25 \
    -rounds 10000 \
    -concurrency $1 
