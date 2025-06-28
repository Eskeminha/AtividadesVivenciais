# 🖥️ Processamento Gráfico — UNISINOS

![Build](https://img.shields.io/badge/build-manual-lightgrey)
![License](https://img.shields.io/badge/license-Acadêmico-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-informational)
![CMake](https://img.shields.io/badge/CMake-%3E=3.10-blue)
![Status](https://img.shields.io/badge/status-em%20desenvolvimento-yellow)

## 📑 Sumário
- [Professora](#-professora)
- [Alunos](#-alunos)
- [Objetivo do Repositório](#-objetivo-do-repositório)
- [Informações adicionais](#-informações-adicionais)
- [Pré-requisitos](#-pré-requisitos)
- [Como rodar](#-como-rodar)
- [FAQ](#-faq)
- [Observações](#-observações)
- [Licença](#-licença)

Repositório utilizado para a **entrega de trabalhos** da disciplina **Processamento Gráfico**, do curso de **Ciência da Computação** da **Universidade do Vale do Rio dos Sinos (UNISINOS)**.

## 👨‍🏫 Professora

**Rossana Baptista Queiroz**  
GitHub: [@fellowsheep](https://github.com/fellowsheep/)

## 👨‍💻 Alunos

- Anderson Koefender  
- Lucas Luan Rost. GitHub: [@LucasRost](https://github.com/LucasRost)
- Raphael Ferracioli. GitHub: [@RaphaH1](https://github.com/RaphaH1)

## 🎯 Objetivo do Repositório

Este repositório contém os trabalhos desenvolvidos ao longo da disciplina. Cada entrega segue os critérios definidos pela professora e será organizada conforme as instruções passadas em aula.


## 📌 Informações adicionais

- **Curso:** Ciência da Computação  
- **Universidade:** UNISINOS  
- **Semestre:** *2025/A*

*Este repositório tem fins exclusivamente acadêmicos.*

## 📦 Pré-requisitos

Antes de tudo, verifique se você possui instalado em sua máquina:

- **CMake** (versão ≥ 3.10)  
- **Compiler C++ compatível** (GCC, Clang, MSVC)  
- Bibliotecas e dependências OpenGL:
  - GLFW  
  - GLAD  
  - GLM  
  - stb_image / stb_easy_font  
- **Windows**: MinGW-Make ou Visual Studio (MSVC)  
- **Linux/macOS**: make ou ninja (conforme o gerador escolhido)


## 🕹️ Como rodar:

Clone o repositório.

Abra o repositório pelo VSCode.

Pressione CTRL + SHIFT + P.

Selecione CMake Configure e selecione o seu compilador.

Assim que configurado, acesse a pasta build e digite:

    cmake --build .

Após gerar os arquivos executáveis, escolha o programa desejado e digite:

**No Windows:**

    ./Nome_Do_Arquivo.exe

**No Linux/macOS:**

    ./Nome_Do_Arquivo

---

## ❓ FAQ

- **Erro: 'GLFW/glfw3.h' not found**
  - Verifique se a biblioteca GLFW está instalada e corretamente referenciada no CMakeLists.txt.
- **Erro: 'stb_image.h' not found**
  - Baixe o arquivo em https://github.com/nothings/stb e coloque em `include/`.
- **Problemas com CMake**
  - Confirme se a versão instalada é ≥ 3.10 e se o caminho está no PATH do sistema.

## ⚠️ Observações

- Este repositório é **exclusivamente acadêmico**.  
- Mantenha sempre o template original e os comentários fornecidos pelo corpo docente.  
- Problemas de compilação geralmente estão relacionados a dependências faltantes ou versão incorreta do CMake.

## 📝 Licença

Uso estritamente acadêmico, conforme diretrizes da disciplina. Não utilize para fins comerciais ou de distribuição pública.

