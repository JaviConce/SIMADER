#!/bin/bash
# Iniciar Node-RED desde esta carpeta

cd "$(dirname "$0")"

npx node-red --userDir .
