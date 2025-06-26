#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
// bloco leds
#include <Adafruit_NeoPixel.h>

#define pinoLed 38
#define numLeds 150


// encerramento bloco leds





#define NUM_LINHAS 7
#define NUM_COLUNAS 7
#define TAM_CELULA 30
#define OFF_Y 5
#define OFF_X 15

TouchScreen touch(6, A1, A2, 7, 300);
const int TS_LEFT = 145, TS_RT = 887, TS_TOP = 934, T;
MCUFRIEND_kbv tela;

int cor1 = 0;
int cor2 = 0;
int cor3 = 0;



// (suas definições de LED devem estar antes; aqui assumo que você já tem numLeds, pinoLed e fita definidos)

int tabuleiro[7][7] = {
  {4, 4, 5, 3, 2, 1, 11},
  { 0, 2, 0, 0, 0, 9,  0},
  { 0, 0, 5, 0,10, 0,  0},
  { 4, 0, 0, 0, 0, 0,  1},
  { 0, 0, 6, 0, 7, 0,  0},
  { 0, 8, 0, 0, 0,12,  0},
  {11, 0, 0,11, 0, 0, 11}
};

const int  INI_FLUX               = 1;  // aguardando comando inicial
const int EST_AGUARDANDO_DESTINO = 2;  // origem escolhida, espera destino
int estado = INI_FLUX;



int LinOri, ColOri;       // casa de origem
int linDest, colDest;     // casa de destino
int possCount;            // número de destinos válidos encontrados
int possMoves[4][2];     
int matrizP1[7][7];
int matrizP2[7][7];

bool vezP1 = true;
bool vence = false;
bool inicio = false;

// bloco leds
int idxLaguinho = 39;
piscarAtivo[numLeds] = { false };
bool piscaRed[ numLeds ]   = { false };
bool piscaGreen[ numLeds ] = { false };
unsigned long ultimaTroca = 0;
bool estadoPisca = false;

Adafruit_NeoPixel fita = Adafruit_NeoPixel(numLeds, pinoLed, NEO_GRB + NEO_KHZ800);
// encerramento bloco leds

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);
  tela.begin( tela.readID() );
  // 1) Fundo e borda
  tela.fillScreen(TFT_DARKBLUE);
  tela.drawRoundRect(10, 10, 300, 220, 20, TFT_YELLOW);
  tela.fillRoundRect(12, 12, 296, 216, 20, TFT_BLUE);

  // 2) Ícone de espada no topo
  int cx = tela.width() / 2;
  int cy = 80;
  // Ponta da espada
  tela.fillTriangle(cx, cy - 30, cx - 8, cy, cx + 8, cy, TFT_SILVER);
  // Lâmina
  tela.fillRect(cx - 4, cy, 8, 70, TFT_SILVER);
  // Guarda da espada
  tela.fillRect(cx - 20, cy + 65, 40, 8, TFT_ORANGE);
  // Cabo da espada
  tela.fillRect(cx - 6, cy + 73, 12, 15, TFT_BROWN);
  // Pommel (bucha no cabo)
  tela.drawCircle(cx, cy + 90, 6, TFT_GOLD);

  // 3) Título principal
  tela.setTextColor(TFT_YELLOW);
  tela.setTextSize(4);
  String title = "MICROMBATE";
  int16_t tx = (tela.width() - title.length() * 18) / 2; // ~18px por char em size=4
  tela.setCursor(tx, 160);
  tela.print(title);

  // 4) Instrução
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(2);
  String instr = "Digite \"iniciar\"";
  tx = (tela.width() - instr.length() * 12) / 2;          // ~12px por char em size=2
  tela.setCursor(tx, 200);
  tela.print(instr);
  geraMatriz();

  // bloco led
  pinMode(pinoLed, OUTPUT);
  fita.begin();
  fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));  // azul
  fita.show();
  // encerramento bloco led
}

void loop() {
  if (Serial.available()) {                        // Verifica se há dados disponíveis na Serial
    String texto = Serial.readStringUntil('\n');  // Lê até a próxima quebra de linha
    texto.trim();                                  // Remove espaços em branco extras

    // 1) Comando para limpar os LEDs
    if (texto.equalsIgnoreCase("clear")) {
      limparMatrizDeLEDs();
      Serial.println("Todos os LEDs foram apagados.");
      estado = INI_FLUX;
      return;
    }

    // 2) Se estiver aguardando o destino da peça
    if (estado == EST_AGUARDANDO_DESTINO) {
      char letra = texto.charAt(0);
      colDest = toupper(letra) - 'A';
      linDest = texto.substring(1).toInt() - 1;

      bool ok = false;
      for (int i = 0; i < possCount; i++) {
        if (possMoves[i][0] == linDest && possMoves[i][1] == colDest) {
          ok = true;
          break;
        }
      }

      if (!ok) {
        Serial.println("Destino inválido. Tente novamente.");
        Serial.println("Para onde?");
        return;
      }

      int peca = tabuleiro[LinOri][ColOri];        // Peça a ser movida
      int casa = tabuleiro[linDest][colDest];      // Casa de destino

      if (casa == 0) {                              // Movimento normal
        move(LinOri, ColOri, linDest, colDest, peca);
        Serial.println("Peça movida com sucesso!");
      } else {
        int resultado = ataque(peca, casa);        // Confronto
        if (resultado == 1)        move(LinOri, ColOri, linDest, colDest, peca);  // Venceu
        else if (resultado == -1) morte(LinOri, ColOri);                          // Perdeu
        else if (resultado == 2)  vence = true;                                    // Ganhou o jogo
        else                      { morte(LinOri, ColOri); morte(linDest, colDest); } // Empate
      }

      limparMatrizDeLEDs();                         // Limpa os LEDs após jogada
      geraMatriz();                                 // Atualiza as matrizes visuais
      exibeMat(vezP1 ? matrizP1 : matrizP2);        // Mostra a matriz correta para o jogador
      imprimeTabuleiro();                           // Mostra no Serial
      if (vence) vencedor();                        // Finaliza se alguém ganhou
      vezP1 = !vezP1;                               // Alterna turno
      estado = INI_FLUX;                            // Reinicia estado
      return;
    }

    // 3) Comando para iniciar o jogo
    if (texto.startsWith("iniciar")) {
      inicio = true;
      vence = false;
      tela.fillScreen(TFT_BLACK);
      exibeMat(matrizP1);
      return;
    }

    // 4) Comandos de controle de LEDs via Serial
    int barra1 = texto.indexOf('/');
    if (barra1 != -1) {
      String acao = texto.substring(0, barra1);
      String coordenadas = texto.substring(barra1 + 1);
      int virgula = coordenadas.indexOf(',');
      if (virgula == -1) {
        Serial.println("Coordenadas inválidas. Use linha,coluna");
        return;
      }
      int linha = coordenadas.substring(0, virgula).toInt();
      int coluna = coordenadas.substring(virgula + 1).toInt();
      if (linha < 0 || linha > 6 || coluna < 0 || coluna > 6) {
        Serial.println("Linha ou coluna fora dos limites.");
        return;
      }
      int idx = posicaoParaIndice(linha, coluna);
      if (idx == idxLaguinho) {
        Serial.println("LED protegido: laguinho");
        return;
      }
      if (acao == "acender") {
        acender(linha, coluna, 1);
      } else if (acao == "apagar") {
        piscarAtivo[idx] = false;
        fita.setPixelColor(idx, 0);
        fita.show();
        Serial.println("LED apagado.");
      } else if (acao == "piscar") {
        piscarAtivo[idx] = true;
        fita.setPixelColor(idx, fita.Color(cor1, cor2, cor3));
        Serial.println("LED piscando.");
      } else {
        Serial.println("Ação inválida. Use: acender, apagar ou piscar");
      }
      return;
    }

    // 5) Comando para mover uma peça
    if (texto.startsWith("mover") && estado == INI_FLUX) {
      char letra = texto.charAt(6);
      int col = toupper(letra) - 'A';
      int lin = texto.substring(7, 8).toInt() - 1;
      String player = texto.substring(9);
      player.trim();

      if (!inicio) {
        Serial.println("Precisa iniciar jogo: iniciar micrombate");
        return;
      }

      int peca = tabuleiro[lin][col];
      bool pecaP1 = (player == "P1" && peca >= 1 && peca <= 5);
      bool pecaP2 = (player == "P2" && peca >= 6 && peca <=   0);
      if ((pecaP1 && !vezP1) || (pecaP2 && vezP1) || !(pecaP2 || pecaP1)) {
        Serial.println("Peca invalida para voce");
        return;
      }

      // calcula os 4 vizinhos
      possCount = 0;
      const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
      for (int d = 0; d < 4; d++) {
        int nl = lin + dirs[d][0];
        int nc = col + dirs[d][1];
        if (nl < 0 || nl >= NUM_LINHAS || nc < 0 || nc >= NUM_COLUNAS) continue;
        int dest = tabuleiro[nl][nc];
        if (dest == 11) continue;
        if (player == "P1") {
          if (dest >= 1 && dest <= 5) continue;
          possMoves[possCount][0] = nl;
          possMoves[possCount][1] = nc;
          possCount++;

          if (dest == 0) piscaVerde(nl, nc);     // Casa vazia → pisca verde
          else           piscaVermelho(nl, nc);   // Inimigo → acende vermelho
        } else if (player == "P2") {
          if (dest >= 6 && dest <= 10) continue;
          possMoves[possCount][0] = nl;
          possMoves[possCount][1] = nc;
          possCount++;

          if (dest == 0) pisca(nl, nc, 2);
          else           acender(nl, nc, 5);
        }
      }

      if (possCount == 0) {
        Serial.println("Sem movimentos possíveis , escolha uma nova peça para mover.");
        estado = INI_FLUX;
        return;
      }

      limparMatrizDeLEDs();
      LinOri = lin;
      ColOri = col;
      acender(LinOri, ColOri, 3);             // Destaca a peça escolhida com amarelo
      Serial.println("Para onde?");
      estado = EST_AGUARDANDO_DESTINO;
      return;
    }

    // 6) Outros comandos
    if (texto.startsWith("imprime")) {
      geraMatriz();
      exibeMat(tabuleiro);
    }
    else if (texto.startsWith("{{")) {
      texto.replace("{", "");
      texto.replace("}", "");
      geraMatriz();
      int linha = 0, coluna = 0, idx = 0;
      while (idx < texto.length() && linha < 7) {
        int espaco = texto.indexOf(',', idx);
        if (espaco == -1) espaco = texto.length();
        String peca = texto.substring(idx, espaco);
        peca.trim();
        int valor = peca.toInt();
        tabuleiro[linha][coluna++] = valor;
        if (coluna == 7) {
          coluna = 0;
          linha++;
          Serial.println("atualizado");
        }
        idx = espaco + 1;
      }
      Serial.println("Matriz recebida!");
      for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
          Serial.print(tabuleiro[i][j]);
          Serial.print(" ");
        }
        Serial.println();
      }
    }
  }

  // 7) Lógica de piscar os LEDs
  if (millis() - ultimaTroca >= 300) {
    ultimaTroca = millis();
    estadoPisca = !estadoPisca;
    for (int i = 0; i < numLeds; i++) {
      if (piscarAtivo[i]) {
        fita.setPixelColor(i, estadoPisca ? fita.Color(cor1, cor2, cor3) : 0);
      }
      else if (piscaRed[i]) {
      // alterna vermelho on/off
     
      fita.setPixelColor(i, estadoPisca ? fita.Color(255,0,0) : 0);
    }
      else if (piscaGreen[i]) {
      // alterna verde on/off
      
      fita.setPixelColor(i, estadoPisca ? fita.Color(0,255,0) : 0);
    }
    }
    fita.show();
  }
}
// fim do loop()


void imprimeTabuleiro(){
  for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
          Serial.print(tabuleiro[i][j]);
          Serial.print(" ");
        }
        Serial.println();
      }
 
}
void move(int linOri, int colOri, int linDest, int colDest, int peca) {
  tabuleiro[linOri][colOri] = 0;
  tabuleiro[linDest][colDest] = peca;
  return;
}

void morte(int linOri, int colOri) {
  tabuleiro[linOri][colOri] = 0;
  return;
}

int ataque(int ata, int def) {
  if (ata >= 6) {
    ata -= 5;
  } else {
    def -= 5;
  }

  int resultado = def - ata;

  if (def == 3 && ata != 4) {
    Serial.println("Você perdeu, pisou em uma mina terrestre!");
    return 0;
  } else if (def == 1) {
    Serial.println("Você ganhou o jogo, parabéns!");
    return 2;
  } else if (ata == def) {
    Serial.println("Os dois foram de base!");
    return 0;
  } else if (ata == 2 && def == 5) {
    Serial.println("Seu general foi morto!");
    return -1;
  } else if (resultado < 0) {
    Serial.println("Você ganhou o ataque!");
    return 1;
  } else if (resultado > 0) {
    Serial.println("Você perdeu o ataque!");
    return -1;
  }

  // Se nenhum caso acima for satisfeito, retornar 0 por padrão
  return 0;
}

int letraParaColuna(char letra) {
  return toupper(letra) - 'A';
}


// display


void geraMatriz() {
  int peca;
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      peca = tabuleiro[i][j];

      if (peca == 0 || peca == 11) {
        matrizP1[i][j] = peca;
        matrizP2[i][j] = peca; // vazio ou lago
      }
      else if (peca <= 5) {
        matrizP1[i][j] = peca;
        matrizP2[i][j] = 12; // peça do jogador
      }
      else if (peca >= 6) {
        matrizP1[i][j] = 12;
        matrizP2[i][j] = peca;
      }
     
     
    }
  }
}





uint16_t corDaPeca(int valor) {
  switch (valor) {
    case 0:  return TFT_BLACK;     // vazio
    case 1:  return TFT_BLUE;      // prisioneiro P1
    case 2:  return TFT_GREEN;     // agente secreto P1
    case 3:  return TFT_YELLOW;    // mina terrestre P1
    case 4:  return TFT_ORANGE;    // cabo P1
    case 5:  return TFT_RED;       // general P1
    case 6:  return TFT_NAVY;      // prisioneiro P2
    case 7:  return TFT_DARKGREEN; // agente secreto P2
    case 8:  return TFT_OLIVE;     // mina terrestre P2
    case 9:  return TFT_MAROON;    // cabo P2
    case 10: return TFT_GREENYELLOW;   // general P2
    case 11: return TFT_CYAN;      // lago
    case 12: return TFT_DARKGREY;  // peça oculta
    default: return TFT_MAGENTA;   // erro / indefinido
  }
}


void exibeMat(int matriz[NUM_LINHAS][NUM_COLUNAS]) {
  // desenha grade e preenche células
  for (int i = 0; i < NUM_LINHAS; i++) {
    for (int j = 0; j < NUM_COLUNAS; j++) {
      int x = OFF_X + j * TAM_CELULA;
      int y = OFF_Y + i * TAM_CELULA;

      // borda da célula
      tela.drawRect(x, y, TAM_CELULA, TAM_CELULA, TFT_WHITE);  

      // cor de fundo conforme peça
      uint16_t cor = corDaPeca(matriz[i][j]);
      tela.fillRect(x+1, y+1, TAM_CELULA-2, TAM_CELULA-2, cor);

      // opcional: escrever número no centro da célula
      tela.setCursor(x + TAM_CELULA/2 - 6, y + TAM_CELULA/2 - 8);
      tela.setTextColor(TFT_WHITE);
      tela.setTextSize(2);
      tela.print(matriz[i][j]);
    }
  }
}


void vencedor(){
  tela.fillScreen(TFT_BLACK);
  tela.setCursor(20, 100);
  tela.setTextColor(TFT_GREEN);
  tela.setTextSize(3);
  tela.print("Voce venceu o Microbate");

  tela.setCursor(20, 200);
  tela.setTextColor(TFT_RED);
  tela.setTextSize(2);
  tela.print("Parabens!!");




}

void vencedor2() {
  // 1) Fundo escuro e borda arredondada
  tela.fillScreen(TFT_NAVY);
  tela.drawRoundRect(10, 10, 300, 220, 15, TFT_YELLOW);
  tela.fillRoundRect(12, 12, 296, 216, 15, TFT_DARKBLUE);

  // 2) Título principal em amarelo
  tela.setTextColor(TFT_YELLOW);
  tela.setTextSize(5); // tamanho grande
  // centraliza horizontalmente: (largura_total - largura_texto) / 2
  int16_t textWidth = 10 * 24; // "PARABÉNS!" tem 9 caracteres + exclamação = 10; cada caractere ~24px em size=5  
  int16_t x = (tela.width() - textWidth) / 2;
  tela.setCursor(x,  thirty_five); // y = 35px
  tela.print("PARABÉNS!");

  // 3) Subtítulo em branco logo abaixo
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(3);
  textWidth = 11 * 14; // "VOCÊ VENCEU" = 11 chars; cada ~14px em size=3
  x = (tela.width() - textWidth) / 2;
  tela.setCursor(x, 100);
  tela.print("VOCE VENCEU");

  // 4) Desenha um troféu simples no centro
  // Base do troféu
  int cx = tela.width() / 2;
  tela.fillRect(cx - 30, 140, 60, 40, TFT_YELLOW);
  // Pedestal
  tela.fillRect(cx - 20, 180, 40, 15, TFT_YELLOW);
  // Alças (círculos vazados)
  tela.drawCircle(cx - 40, 160, 10, TFT_YELLOW);
  tela.drawCircle(cx + 40, 160, 10, TFT_YELLOW);
}


//bloco leds

int posicaoParaIndice(int linha, int coluna) {
  int matriz[7][7] = {
    { 0, 1, 2, 3, 4, 5, 6 },
    { 18, 17, 16, 15, 14, 13, 12 },
    { 24, 25, 26, 27, 28, 29, 30 },
    { 42, 41, 40, 39, 38, 37, 36 },
    { 48, 49, 50, 51, 52, 53, 54 },
    { 66, 65, 64, 63, 62, 61, 60 },
    { 72, 73, 74, 75, 76, 77, 78 }
  };
  return matriz[linha][coluna];
}

void corDoLed(int cor) {
  switch (cor) {
    case 0:  cor1 = 0;   cor2 = 0;   cor3 = 0;   break; // vazio (apagado)
    case 1:  cor1 = 0;   cor2 = 0;   cor3 = 147; break; // prisioneiro P1 → rosa choque
    case 2:  cor1 = 0;   cor2 = 255; cor3 = 0;   break; // agente secreto P1 → verde
    case 3:  cor1 = 255; cor2 = 255; cor3 = 0;   break; // mina terrestre P1 → amarelo
    case 4:  cor1 = 255; cor2 = 165; cor3 = 0;   break; // cabo P1 → laranja
    case 5:  cor1 = 255; cor2 = 0;   cor3 = 0;   break; // general P1 → vermelho
    case 6:  cor1 = 0;   cor2 = 0;   cor3 = 128; break; // prisioneiro P2 → marinho
    case 7:  cor1 = 0;   cor2 = 100; cor3 = 0;   break; // agente secreto P2 → verde escuro
    case 8:  cor1 = 128; cor2 = 128; cor3 = 0;   break; // mina terrestre P2 → oliva
    case 9:  cor1 = 128; cor2 = 0;   cor3 = 0;   break; // cabo P2 → marrom
    case 10: cor1 = 173; cor2 = 255; cor3 = 47;  break; // general P2 → verde-limão
    case 11: cor1 = 169; cor2 = 169; cor3 = 169; break; // peça oculta → cinza escuro
    default: cor1 = 255; cor2 = 0;   cor3 = 255; break; // erro → magenta
  }
}


void acender(int linha, int coluna, int cor) {
  
  int idx = posicaoParaIndice(linha, coluna);
  piscarAtivo[idx] = false;
  corDoLed(cor);
  Serial.println(idx);
  fita.setPixelColor(idx, fita.Color(cor1, cor2, cor3));
  //fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));   // inutil
  fita.show();
  Serial.println("LED aceso.");
}


void limparMatrizDeLEDs() {
  for (int i = 0; i < numLeds; i++) {
    fita.setPixelColor(i, 0);
    piscarAtivo[i] = false;
  }
  fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));
  fita.show();
}


// faz o LED da (linha,coluna) piscar em vermelho
void piscaVermelho(int linha, int coluna) {
  int idx = posicaoParaIndice(linha, coluna);
  piscaRed[idx] = true;             // marca para piscar vermelho
  piscaGreen[idx] = false;          // garante que não está marcado verde
  // acende de cara em vermelho
                     
  fita.setPixelColor(idx, fita.Color(255,0,0));
  fita.show();
}

// faz o LED da (linha,coluna) piscar em verde
void piscaVerde(int linha, int coluna) {
  int idx = posicaoParaIndice(linha, coluna);
  piscaGreen[idx] = true;           // marca para piscar verde
  piscaRed[idx] = false;            // garante que não está marcado vermelho
  // acende de cara em verde
              
  fita.setPixelColor(idx, fita.Color(0,255,0));
  fita.show();
}

void clearBlink() {
  for (int i = 0; i < numLeds; i++) {
    piscaRed[i]   = false;
    piscaGreen[i] = false;
    fita.setPixelColor(i, 0);
  }
  fita.show();
}
//encerramento bloco leds
