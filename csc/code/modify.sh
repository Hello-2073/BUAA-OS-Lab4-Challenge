# bash modify.sh fibo.c char int
# replace all "char" in file fibo.c with "int"
sed -i 's/'${2}'/'${3}'/g' ${1} 
