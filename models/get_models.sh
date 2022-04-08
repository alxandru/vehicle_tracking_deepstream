#!/bin/bash

python3 -c "import gdown"

if [[ $? = "1" ]]; then
    echo "*** Installing gdown..."
    pip3 install gdown
fi

echo "*** Donwloading YOLOv4 custom model..."
python3 -m gdown.cli https://drive.google.com/uc?id=1fP5B6XjzaWdJjLl18hAElP0pIlUjhyfH

echo "*** Donwloading mars-small128 model..."
python3 -m gdown.cli https://drive.google.com/uc?id=1zZ9joCdN3LBBO04FNP8ZRow3lmHqplly
