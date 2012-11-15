#! /bin/bash
cd
cd Документы/OS/Client-Server
gcc  Client.c -o cln
for ((i = 0;i < 2;i++)); do
        ./cln&
done
