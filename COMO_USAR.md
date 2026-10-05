# zenonia_nx — port do ZENONIA 1 (primeira versão, ainda não testada)

Estes arquivos são a parte do **jogo** no método android32. O `runtime/` (android32),
o `portlibs32/` (mesa32) e o libnx32 continuam sendo os que você já montou.

## 1. Copiar para o Codespace

1. No painel de arquivos do Codespace, arraste o conteúdo desta pasta (`source/`, `tools/`,
   `launcher/`, `Makefile`, `build.sh`, `.gitignore`) para dentro de `Zenonia_nx/`.
   Se perguntar se deve substituir o `.gitignore`, pode substituir.
2. No terminal:

```bash
cd /workspaces/Zenonia_nx
chmod +x build.sh launcher/build.sh
```

## 2. Compilar o jogo (32 bits)

```bash
./build.sh
```

Se compilar, aparecem `zenonia_nx.nsp` e `zenonia_nx.build` na pasta do projeto.
Se der erro, copie as **últimas 40 linhas** do terminal e me mande.

## 3. Compilar o launcher (o .nro)

```bash
launcher/build.sh
```

O arquivo `zenonia_nx.nro` aparece dentro de `launcher/`. Baixe pelo painel de arquivos
(botão direito → Download).

## 4. No Switch

1. Crie `sd:/switch/zenonia_nx/` e coloque nela o `zenonia_nx.nro` e o APK do Zenonia
   (qualquer nome terminado em `.apk`).
2. No sphaira: Homebrew → ZENONIA → Install Forwarder.
3. Abra o ícone novo na tela HOME. A primeira abertura reinicia e desempacota a biblioteca
   do jogo (uma vez, com barra de progresso).
4. Se algo der errado, pegue `debug.log` e `crash.log` na pasta `sd:/switch/zenonia_nx/`
   e me mande.

## Controles (primeira versão)

| Controle | Ação no jogo |
| --- | --- |
| D-pad ou analógico esquerdo | andar (4 direções) |
| A | confirmar / atacar |
| B | menu do jogo / cancelar |
| X | mapa |
| Y | troca de slot / pular |
| L / R | abas do menu (esquerda / direita) |
| ZL, ZR, clique dos analógicos | os 4 slots rápidos |
| Menos | "toque qualquer lugar" (passar textos) |
| Toque na tela | também passa textos |

## O que mais provavelmente precisa de ajuste

- Direções trocadas (herói anda para o lado errado): constantes `DIR_*` em `source/zen_input.c`.
- Toque nos menus: só o toque simples (passar texto) foi ligado; menus por toque não.
- Digitar texto (se o jogo pedir nome): ainda não existe.
- Som: volume e mapeamento dos 51 sons foram deduzidos do código Java; ouvir tudo ajuda a confirmar.
- Velocidade do jogo: ela segue o `SetSpeed` do próprio jogo (25 quadros por segundo no início).
