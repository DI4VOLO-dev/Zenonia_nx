# Zenonia 3 — versão 1.0.1 (corrige as telas de título e do menu que ficavam escuras/brancas)

Causa: na 1.0.0 o ajuste que esconde o analógico e o botão de bater escondia TODOS os objetos de
interface do jogo, e no Zenonia 3 a tela de título e o menu inicial são feitos com esses mesmos
objetos. Agora só os dois objetos do controle de toque (analógico e botão de bater) são escondidos.

```bash
cd /workspaces/Zenonia_nx
git branch                      # deve mostrar * zenonia3
unzip -o zenonia3_v101_update.zip
cp -r zenonia3_nx/. . && rm -rf zenonia3_nx zenonia3_v101_update.zip
./build.sh
launcher/build.sh
git add . && git commit -m "Zenonia 3 v1.0.1: esconde so o controle de toque" && git push
```

Leve o novo `launcher/zenonia3_nx.nro` para `sd:/switch/zenonia3_nx/` (substitui o antigo) e, no
sphaira, rode "Install Forwarder" de novo se quiser que o ícone novo apareça na tela HOME.

No `debug.log` deve aparecer: `[patch] the game's on-screen pad is hidden (IsShow at ... pad vptrs ...)`.
