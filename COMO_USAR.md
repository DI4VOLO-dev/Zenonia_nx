# zenonia2_nx — port do ZENONIA 2 (primeira versão, ainda não compilada nem testada)

Estes arquivos substituem os do Zenonia 1 **dentro da ramificação `zenonia2`**.
O `runtime/` (android32), o `portlibs32/` (mesa32) e o libnx32 continuam os mesmos.

## 1. Conferir que você está na ramificação certa

```bash
cd /workspaces/Zenonia_nx
git branch
```

Deve aparecer `* zenonia2`. Se aparecer `* main`, rode `git checkout zenonia2`.

## 2. Colocar os arquivos novos por cima

1. Arraste o `zenonia2_nx_port.zip` para o painel de arquivos, na raiz de `Zenonia_nx`.
2. No terminal:

```bash
cd /workspaces/Zenonia_nx
unzip -o zenonia2_nx_port.zip
cp -r zenonia2_nx/. .
rm -rf zenonia2_nx zenonia2_nx_port.zip
chmod +x build.sh launcher/build.sh
```

(O `cp -r ... .` copia também o `.gitignore`, que é um arquivo oculto.)

## 3. Limpar o que sobrou do Zenonia 1 e compilar

```bash
rm -rf build launcher/build *.nsp *.build launcher/*.nro
./build.sh
```

Se compilar, aparecem `zenonia2_nx.nsp` e `zenonia2_nx.build`. Depois:

```bash
launcher/build.sh
```

O `launcher/zenonia2_nx.nro` é o arquivo que vai para o Switch (botão direito → Download).
Se algum comando falhar, copie as **últimas 40 linhas** do terminal e me mande.

## 4. Salvar o progresso

```bash
git add .
git commit -m "Zenonia 2: primeira versão"
git push -u origin zenonia2
```

## 5. No Switch

1. Crie `sd:/switch/zenonia2_nx/` e coloque nela o `zenonia2_nx.nro` e o APK do Zenonia 2
   (qualquer nome terminado em `.apk`).
2. No sphaira: Homebrew → ZENONIA 2 → Install Forwarder.
3. Abra o ícone novo na tela HOME. A primeira abertura desempacota a biblioteca do jogo.
4. Se algo der errado, pegue `debug.log` e `crash.log` na pasta `sd:/switch/zenonia2_nx/`.

Ele convive com o Zenonia 1 (outra pasta, outro ícone, outro ID de programa: 0x0100000000001021).

## Controles

| Controle | Ação |
| --- | --- |
| D-pad ou analógico esquerdo | andar (os 4 sentidos; dois juntos fazem diagonal) |
| A | confirmar / atacar |
| B ou + | menu do jogo |
| − ou L | voltar / cancelar |
| X | mapa |
| Y ou R | troca de habilidade |
| ZL, ZR, clique dos analógicos | os 4 slots rápidos |
| Toque na tela | toque do jogo (menus, NPCs, botões da tela) |

## O que mais provavelmente precisa de ajuste

- O jogo desenha os próprios botões na tela e muitos menus dependem de **toque**: no modo portátil
  funciona direto; no dock não há como tocar (me avise se isso atrapalhar e eu penso num cursor).
- O código do botão de mapa (X) e os de L/R são deduzidos do código Java, não foram vistos funcionando.
- A velocidade do jogo vem do próprio jogo (30 quadros por segundo até ele pedir outra).
- Se aparecerem telas de loja, login ou termos coreanos, me diga: são do APK e dá para contornar.
