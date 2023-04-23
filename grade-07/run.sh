#!/bin/bash

if [[ $# -ne 3 ]] ; 
then
    echo "You should pass 3 args: field_size, work_time_1, work_time_2"
    exit 1
fi

./first_gardener $1 &
./second_gardener $2 &
./map_handler $3
