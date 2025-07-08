#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
// bloco leds
#include <Adafruit_NeoPixel.h>

#define pinoLed 38
#define numLeds 200

#define TFT_GOLD 0xFEA0
#define TFT_SILVER 0xC618
#define TFT_BRONZE 0xCA60
#define TFT_NEON_GREEN 0x07F0
#define TFT_NEON_BLUE 0x1F9F
#define TFT_NEON_PINK 0xF8BF
#define TFT_BEIGE 0xF7BB
#define TFT_MAROON 0x8000
#define TFT_NAVY 0x000F
#define TFT_TEAL 0x0410
#define TFT_OLIVE 0x8400
#define TFT_CORAL 0xFBEA
#define TFT_ORANGE 0xFD20
#define TFT_PINK 0xF81F
#define TFT_PURPLE 0x780F
#define TFT_BROWN 0xA145
#define TFT_GRAY 0x8410
#define TFT_LIGHTGRAY 0xC618
// encerramento bloco leds






#define NUM_LINHAS 7
#define NUM_COLUNAS 7
#define TAM_CELULA 30
#define OFF_Y 5
#define OFF_X 15

void geraMatriz();


TouchScreen touch(6, A1, A2, 7, 300);
const int TS_LEFT = 145, TS_RT = 887, TS_TOP = 934, T;
MCUFRIEND_kbv tela;

int cor1 = 0;
int cor2 = 0;
int cor3 = 0;



// (suas definições de LED devem estar antes; aqui assumo que você já tem numLeds, pinoLed e fita definidos)

int tabuleiro[7][7] = { 0 };
int tabuleiroNovo[7][7];

const int INI_FLUX = 1;                // aguardando comando inicial
const int EST_AGUARDANDO_DESTINO = 2;  // origem escolhida, espera destino
int estado = INI_FLUX;
//logica nova
int linzero;
int colzero;


//logica antiga
int LinOri, ColOri;    // casa de origem
int linDest, colDest;  // casa de destino
int possCount;         // número de destinos válidos encontrados
int possMoves[4][2];
int matrizP1[7][7];
int matrizP2[7][7];
int pecaRemovida;      // peca removida do tabuleiro para ser movida
bool vezP1 = true;
bool vence = false;

bool inicio = false;

// bloco leds
int idxLaguinho = 39;
bool piscarAtivo[numLeds] = { false };
bool piscaRed[numLeds] = { false };
bool piscaGreen[numLeds] = { false };
unsigned long ultimaTroca = 0;
bool estadoPisca = false;

Adafruit_NeoPixel fita = Adafruit_NeoPixel(numLeds, pinoLed, NEO_GRB + NEO_KHZ800);
// encerramento bloco leds
void geraMatriz();
void imprimeTabuleiro();
void move(int linOri, int colOri, int linDest, int colDest, int peca);
void morte(int linOri, int colOri);
int ataque(int ata, int def);
int letraParaColuna(char letra);
uint16_t corDaPeca(int valor);
void exibeMat(int matriz[7][7]);
void exibeMat2(int matriz[7][7]);
void vencedor();
void vencedor2();
int posicaoParaIndice(int linha, int coluna);
void corDoLed(int cor);
void acender(int linha, int coluna, int cor);
void limparMatrizDeLEDs();
void piscaVerde(int linha, int coluna);
void piscaVermelho(int linha, int coluna);
void clearBlink();

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  tela.begin(tela.readID());


  //  1) Fundo e borda
  tela.fillScreen(TFT_DARKGREY);
  tela.drawRoundRect(10, 10, 300, 220, 20, TFT_YELLOW);
  tela.fillRoundRect(12, 12, 296, 216, 20, TFT_BLUE);
  // 2) Ícone de espada no topo
  int cx = tela.width() / 2;
  int cy = 80;
  // Ponta da espada
  tela.fillTriangle(cx, cy - 30, cx - 4, cy, cx + 4, cy, TFT_SILVER);
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
  tela.setTextSize(3);
  String title = "MICROMBATE";
  int16_t tx = (tela.width() - title.length() * 18) / 2;  // ~18px por char em size=4
  tela.setCursor(tx, 160);
  tela.print(title);
  // 4) Instrução
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(2);
  String instr = "Digite \"iniciar\"";
  tx = (tela.width() - instr.length() * 12) / 2;  // ~12px por char em size=2
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
  if (Serial1.available()) {
    String recebido = Serial1.readStringUntil('\n');
    recebido.trim();

    // 1) Atualização de matriz via "{{...}}"
    if (recebido.startsWith("{{")) {
      recebido.replace("{", "");
      recebido.replace("}", "");
      geraMatriz();
      int linha = 0, coluna = 0, idx = 0;
      if (tabuleiro[4][4] != 11) {
        while (idx < recebido.length() && linha < NUM_LINHAS) {
          int espaco = recebido.indexOf(',', idx);
          if (espaco == -1) espaco = recebido.length();
          tabuleiro[linha][coluna++] = recebido.substring(idx, espaco).toInt();
          if (coluna == NUM_COLUNAS) {
            coluna = 0; linha++;
            Serial.println("matriz inicial feita");
          }
          idx = espaco + 1;
        }
      } else {
        while (idx < recebido.length() && linha < NUM_LINHAS) {
          int espaco = recebido.indexOf(',', idx);
          if (espaco == -1) espaco = recebido.length();
          tabuleiroNovo[linha][coluna++] = recebido.substring(idx, espaco).toInt();
          if (coluna == NUM_COLUNAS) {
            coluna = 0; linha++;
            Serial.println("atualizado");
          }
          idx = espaco + 1;
        }
      }
      Serial.println("Matriz recebida!");
      imprimeTabuleiro();
    }
    // 2) MUD Xn → remove peça e pisca adjacentes
    else if (recebido.startsWith("MUD ")) {
      limparMatrizDeLEDs();
      ColOri = toupper(recebido.charAt(4)) - 'A';
      LinOri = recebido.substring(5).toInt() - 1;
      pecaRemovida = tabuleiro[LinOri][ColOri];
      tabuleiro[LinOri][ColOri] = 0;

      possCount = 0;
      const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
      for (int d = 0; d < 4; d++) {
        int nl = LinOri + dirs[d][0], nc = ColOri + dirs[d][1];
        if (nl<0||nl>=NUM_LINHAS||nc<0||nc>=NUM_COLUNAS) continue;
        int dest = tabuleiro[nl][nc];
        if (dest == 11) continue;
        possMoves[possCount][0] = nl;
        possMoves[possCount][1] = nc;
        possCount++;
        if (dest == 0) piscaVerde(nl,nc);
        else           piscaVermelho(nl,nc);
      }
      geraMatriz();
      estado = EST_AGUARDANDO_DESTINO;
    }
    // 3) PUT Xn → coloca no destino
    else if (recebido.startsWith("PUT ")) {
      if (estado != EST_AGUARDANDO_DESTINO) {
        Serial.println("Use MUD antes de PUT.");
        return;
      }
      limparMatrizDeLEDs();
      int colDest = toupper(recebido.charAt(4)) - 'A';
      int linDest = recebido.substring(5).toInt() - 1;

      bool ok = false;
      for (int i = 0; i < possCount; i++) {
        if (possMoves[i][0] == linDest && possMoves[i][1] == colDest) {
          ok = true; break;
        }
      }
      if (!ok) {
          
          int msgX  = OFF_X;
          int msgY  = OFF_Y + NUM_LINHAS * TAM_CELULA + 10;
          int msgW  = NUM_COLUNAS * TAM_CELULA;   // largura igual ao tabuleiro
          int msgH  = 2 * 16;                     // duas linhas de texto em size=2 (~16px de altura cada)

          
          tela.fillRect(msgX, msgY, msgW, msgH, TFT_BLACK);

          
          tela.setTextColor(TFT_RED);
          tela.setTextSize(2);
          tela.setCursor(msgX + 2, msgY + 2);
          tela.print("Destino invalido!");
          tela.setCursor(msgX + 2, msgY + 18);
          tela.print("Escolha posicao valida");

          
          return;

      int destino = tabuleiro[linDest][colDest];
      if (destino == 0) {
        move(LinOri, ColOri, linDest, colDest, pecaRemovida);
        Serial.println("Peça posicionada com sucesso!");
      } else {
        int res = ataque(pecaRemovida, destino);
        if (res == 1)      move(LinOri, ColOri, linDest, colDest, pecaRemovida);
        else if (res == -1) morte(LinOri, ColOri);
        else if (res == 2) vence = true;
        else {
          morte(LinOri, ColOri);
          morte(linDest, colDest);
        }
      }

      limparMatrizDeLEDs();
      geraMatriz();
      vezP1 = !vezP1;
      if (vezP1) exibeMat2(matrizP1);
      else       exibeMat2(matrizP2);
      imprimeTabuleiro();
      if (vence) vencedor2();
      estado = INI_FLUX;
    }
  }
  }
  // 4) Timer de piscar NeoPixels
  if (millis() - ultimaTroca >= 300) {
    ultimaTroca = millis();
    estadoPisca = !estadoPisca;
    for (int i = 0; i < numLeds; i++) {
      if (piscaRed[i])    fita.setPixelColor(i, estadoPisca?fita.Color(255,0,0):0);
      else if (piscaGreen[i]) fita.setPixelColor(i, estadoPisca?fita.Color(0,255,0):0);
    }
    fita.show();
  }

}


void imprimeTabuleiro() {
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
        matrizP2[i][j] = peca;  // vazio ou lago
      } else if (peca <= 5) {
        matrizP1[i][j] = peca;
        matrizP2[i][j] = 12;  // peça do jogador
      } else if (peca >= 6) {
        matrizP1[i][j] = 12;
        matrizP2[i][j] = peca;
      }
    }
  }
}
uint16_t corDaPeca(int valor) {
  switch (valor) {
    case 1:
    case 6: return TFT_MAGENTA;  // Prisioneiro
    case 2:
    case 7: return TFT_GREEN;  // Agente secreto
    case 3:
    case 8: return TFT_YELLOW;  // Mina terrestre
    case 4:
    case 9: return TFT_ORANGE;  // Cabo
    case 5:
    case 10: return TFT_RED;       // General
    case 11: return TFT_CYAN;      // Lago
    case 12: return TFT_DARKGREY;  // Peça oculta
    default: return TFT_BLACK;     // Vazio / erro
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
      tela.fillRect(x + 1, y + 1, TAM_CELULA - 2, TAM_CELULA - 2, cor);

      // opcional: escrever número no centro da célula
      tela.setCursor(x + TAM_CELULA / 2 - 6, y + TAM_CELULA / 2 - 8);
      tela.setTextColor(TFT_WHITE);
      tela.setTextSize(2);
      tela.print(matriz[i][j]);
    }
  }
}
void exibeMat2(int matriz[NUM_LINHAS][NUM_COLUNAS]) {
  // 1) Desenha grade e preenche células (sem imprimir número)
  for (int i = 0; i < NUM_LINHAS; i++) {
    for (int j = 0; j < NUM_COLUNAS; j++) {
      int x = OFF_X + j * TAM_CELULA;
      int y = OFF_Y + i * TAM_CELULA;
      tela.drawRect(x, y, TAM_CELULA, TAM_CELULA, TFT_WHITE);
      uint16_t cor = corDaPeca(matriz[i][j]);
      tela.fillRect(x + 1, y + 1, TAM_CELULA - 2, TAM_CELULA - 2, cor);
    }
  }

  // 2) Desenha legenda abaixo do tabuleiro
  const char* labels[] = {
    "Prisioneiro", "Agente", "Mina",
    "Cabo", "General", "Lago"
  };
  const int types[] = { 1, 2, 3, 4, 5, 11 };
  int legendX = OFF_X;
  int legendY = OFF_Y + NUM_LINHAS * TAM_CELULA + 10;
  tela.setTextSize(1);
  tela.setTextColor(TFT_WHITE);

  for (int k = 0; k < 3; k++) {
    uint16_t c = corDaPeca(types[k]);
    // quadradinho colorido
    tela.fillRect(legendX, legendY, 20, 20, c);
    tela.drawRect(legendX, legendY, 20, 20, TFT_WHITE);
    // rótulo ao lado
    tela.setCursor(legendX + 26, legendY + 6);
    tela.print(labels[k]);
    // avança para a próxima entrada da legenda
    legendY += 30;
  }
  legendX = OFF_X;
  legendY = OFF_Y + NUM_LINHAS * TAM_CELULA + 10;
  for (int k = 3; k < 6; k++) {
    uint16_t c = corDaPeca(types[k]);
    // quadradinho colorido
    tela.fillRect(legendX + 100, legendY, 20, 20, c);
    tela.drawRect(legendX + 100, legendY, 20, 20, TFT_WHITE);
    // rótulo ao lado
    tela.setCursor(legendX + 126, legendY + 6);
    tela.print(labels[k]);
    // avança para a próxima entrada da legenda
    legendY += 30;
  }
}
void vencedor() {
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
  tela.fillRoundRect(12, 12, 296, 216, 15, TFT_DARKGREY);

  // 2) Título principal em amarelo
  tela.setTextColor(TFT_YELLOW);
  tela.setTextSize(3);  // tamanho grande
  // centraliza horizontalmente: (largura_total - largura_texto) / 2
  int16_t textWidth = 10 * 24;  // "PARABÉNS!" tem 9 caracteres + exclamação = 10; cada caractere ~24px em size=5
  int16_t x = (tela.width() - textWidth) / 2;
  tela.setCursor(x, 35);  // y = 35px
  tela.print("   PARABENS!");

  // 3) Subtítulo em branco logo abaixo
  tela.setTextColor(TFT_WHITE);
  tela.setTextSize(2);
  textWidth = 11 * 14;  // "VOCÊ VENCEU" = 11 chars; cada ~14px em size=3
  x = (tela.width() - textWidth) / 2;
  tela.setCursor(x, 100);
  
  if(vezP1) tela.print("VOCE VENCEU P1");
  
  else tela.print("VOCE VENCEU P2");
  

  // 4) Desenha um troféu detalhado no centro
  int cx = tela.width() / 2;
  tela.fillRect(cx - 30, 140, 60, 40, TFT_YELLOW);       // Parte principal do troféu
  tela.fillRect(cx - 20, 180, 40, 15, TFT_YELLOW);       // Pedestal

// Alças mais grossas (3 linhas de círculo cada)
  for (int r = 9; r <= 11; r++) {
    tela.drawCircle(cx - 40, 160, r, TFT_YELLOW);
    tela.drawCircle(cx + 40, 160, r, TFT_YELLOW);
}
}
//bloco leds
int posicaoParaIndice(int linha, int coluna) {
  int matriz[7][7] = {
    { 0, 2, 4, 6, 8, 10, 12 },
    { 30, 28, 26, 24, 22, 20, 18 },
    { 36, 38, 40, 42, 44, 46, 48 },
    { 66, 64, 62, 60, 58, 56, 54 },
    { 72, 74, 76, 78, 80, 82, 84 },
    { 102, 100, 98, 96, 94, 92, 90 },
    { 108, 110, 112, 114, 116, 118, 120 }
  };
  return matriz[linha][coluna];
}
void corDoLed(int cor) {
  switch (cor) {
    case 0:
      cor1 = 0;
      cor2 = 0;
      cor3 = 0;
      break;  // vazio (apagado)
    case 1:
      cor1 = 0;
      cor2 = 0;
      cor3 = 147;
      break;  // prisioneiro P1 → rosa choque
    case 2:
      cor1 = 0;
      cor2 = 255;
      cor3 = 0;
      break;  // agente secreto P1 → verde
    case 3:
      cor1 = 255;
      cor2 = 255;
      cor3 = 0;
      break;  // mina terrestre P1 → amarelo
    case 4:
      cor1 = 255;
      cor2 = 165;
      cor3 = 0;
      break;  // cabo P1 → laranja
    case 5:
      cor1 = 255;
      cor2 = 0;
      cor3 = 0;
      break;  // general P1 → vermelho
    case 6:
      cor1 = 0;
      cor2 = 0;
      cor3 = 128;
      break;  // prisioneiro P2 → marinho
    case 7:
      cor1 = 0;
      cor2 = 100;
      cor3 = 0;
      break;  // agente secreto P2 → verde escuro
    case 8:
      cor1 = 128;
      cor2 = 128;
      cor3 = 0;
      break;  // mina terrestre P2 → oliva
    case 9:
      cor1 = 128;
      cor2 = 0;
      cor3 = 0;
      break;  // cabo P2 → marrom
    case 10:
      cor1 = 173;
      cor2 = 255;
      cor3 = 47;
      break;  // general P2 → verde-limão
    case 11:
      cor1 = 169;
      cor2 = 169;
      cor3 = 169;
      break;  // peça oculta → cinza escuro
    default:
      cor1 = 255;
      cor2 = 0;
      cor3 = 255;
      break;  // erro → magenta
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
    piscaRed[i] = false;
    piscaGreen[i] = false;
  }
  fita.setPixelColor(idxLaguinho, fita.Color(0, 0, 255));
  fita.show();
}
// faz o LED da (linha,coluna) piscar em vermelho
void piscaVermelho(int linha, int coluna) {
  int idx = posicaoParaIndice(linha, coluna);
  piscaRed[idx] = true;     // marca para piscar vermelho
  piscaGreen[idx] = false;  // garante que não está marcado verde
  // acende de cara em vermelho

  fita.setPixelColor(idx, fita.Color(255, 0, 0));
  fita.show();
}
// faz o LED da (linha,coluna) piscar em verde
void piscaVerde(int linha, int coluna) {
  int idx = posicaoParaIndice(linha, coluna);
  piscaGreen[idx] = true;  // marca para piscar verde
  piscaRed[idx] = false;   // garante que não está marcado vermelho
  // acende de cara em verde

  fita.setPixelColor(idx, fita.Color(0, 255, 0));
  fita.show();
}
void clearBlink() {
  for (int i = 0; i < numLeds; i++) {
    piscaRed[i] = false;
    piscaGreen[i] = false;
    fita.setPixelColor(i, 0);
  }
  fita.show();
}
//encerramento bloco leds




