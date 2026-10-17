# Zenonia 1 — versão 1.0.4: o analógico esquerdo move o personagem também no modo portátil

Causa: o jogo lia o analógico só do controle "jogador 1". No modo portátil os Joy-Cons acoplados
são outro "slot", e o analógico deles era ignorado (só o direcional funcionava). Agora os dois são lidos.

Na ramificação `main`:

```bash
cd /workspaces/Zenonia_nx
git status                       # deve dizer "nothing to commit"
git checkout main
unzip -o zenonia1_v104_update.zip
cp -r zenonia_nx/. . && rm -rf zenonia_nx zenonia1_v104_update.zip
rm -rf build launcher/build
./build.sh
launcher/build.sh
```

Leve `launcher/zenonia_nx.nro` para `sd:/switch/zenonia_nx/` e teste no modo portátil.
