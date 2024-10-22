# Script to build files

cd ./clientFolder
make

chmod +x ./client

cp ./client ../multipleConnection/
cd ../multipleConnection
chmod +x ./client

cd ../serverFolder/
make
chmod +x ./server
