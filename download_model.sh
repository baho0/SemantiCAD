#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

mkdir -p models
MODEL_FILE="models/Qwen2.5-3B-Instruct-Q4_K_M.gguf"
MODEL_URL="https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf?download=true"

echo -e "${BLUE}[SemantiCAD] Checking for model...${NC}"

if [ -f "$MODEL_FILE" ]; then
    echo -e "${GREEN}Model already exists:${NC} $MODEL_FILE"
else
    echo -e "${BLUE}Downloading Qwen2.5 model (1.1 GB). Please wait...${NC}"
    wget -O "$MODEL_FILE" "$MODEL_URL"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Model downloaded successfully and placed in models/ directory!${NC}"
    else
        echo "An error occurred while downloading the model."
        exit 1
    fi
fi