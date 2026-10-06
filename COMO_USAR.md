# zenonia3_nx — port do ZENONIA 3 (primeira versão, ainda não compilada nem testada)

Mesma engine do Zenonia 1 e 2. Você pode manter os três no Switch (cada um com pasta, ícone
e ID de programa próprios; este usa 0x0100000000001022).

## 1. Salvar o Zenonia 2 e criar a ramificação do 3

```bash
cd /workspaces/Zenonia_nx
git add . && git commit -m "Zenonia 2: v1.0.2"
git push
git tag zenonia2-v1.0.2 && git push --tags
git checkout -b zenonia3
git branch                      # deve mostrar * zenonia3
```

(Se o commit disser "nothing to commit", tudo bem.)

## 2. Colocar os arquivos novos por cima

1. Arraste o `zenonia3_nx_port.zip` para o painel de arquivos, na raiz de `Zenonia_nx`.
2. No terminal:

```bash
cd /workspaces/Zenonia_nx
unzip -o zenonia3_nx_port.zip
cp -r zenonia3_nx/. .
rm -rf zenonia3_nx zenonia3_nx_port.zip
chmod +x build.sh launcher/build.sh
ls source/                      # deve ter config.h, minimp3.h, minimp3_ex.h, zen_*.c ...
```

## 3. Compilar (limpando os objetos do jogo anterior)

```bash
rm -rf build launcher/build
./build.sh
launcher/build.sh
```

Saem `zenonia3_nx.nsp` e `launcher/zenonia3_nx.nro` (botão direito → Download). Os arquivos do
Zenonia 2 (`zenonia2_nx.*`) continuam lá, com outro nome. Se algo falhar, mande as **últimas 40
linhas** do terminal.

## 4. Salvar

```bash
git add . && git commit -m "Zenonia 3: primeira versão" && git push -u origin zenonia3
```

## 5. No Switch

1. Crie `sd:/switch/zenonia3_nx/` com o `zenonia3_nx.nro` e o APK do Zenonia 3 (qualquer nome
   terminado em `.apk`).
2. No sphaira: Homebrew → ZENONIA 3 → Install Forwarder; abra o ícone novo.
3. A primeira abertura desempacota a biblioteca do jogo.
4. Se algo der errado: `debug.log` e `crash.log` da pasta `sd:/switch/zenonia3_nx/`.

## O que este port já traz (herdado do Zenonia 2)

Controles (D-pad/analógico, A, B, X, Y, L, R, ZL, ZR, +, −), toque nos menus, esconde o
analógico e o botão de bater da tela (`config.ini` → `[display] touch_buttons = true` para
voltar), memória corrigida (quarentena de blocos liberados), log sem excesso.

## O que é novo e merece atenção no teste

- **Som em MP3** (o 2 usava WAV): decodificado com o minimp3, na primeira vez que cada som toca.
  Se aparecer silêncio, ou um pequeno engasgo na primeira vez de uma música, me diga.
- **Música em estéreo** (o mixer agora é estéreo).
- Título "ZENONIA 3", autor "Gamevil/DI4VOLO", versão 1.0.0 e o **ícone padrão**: me mande a
  imagem do ícone e a versão que quiser e eu atualizo.
