​//Code Created by Alan Vieira Giavera - 2015

//Copyright  -  https://alanshowreel.wordpress.com


#include <SPI.h>
#include <String.h>
#include <Ethernet.h>
#include <Utility\Socket.h>
#include <IRremote.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x9B, 0x36 }; // Endereço Mac
byte ip[] = { 192, 168, 0, 00 }; // Endereço de Ip da sua Rede
EthernetServer server(80); // Porta de serviço

int lampada1 = 5;
int lampada2 = 6;
int ventilador = 7;

//variaveis para enviar o ir
const int ledIR = 9; //porta do emissor ir 
IRsend irsend; //variável para poder enviar o código a TV 
//codigos brutos raw do ir da tv e som
// Power ON / OFF SOM
unsigned int powerOnSOM[100] = {3300, 1850, 450, 1150, 450, 1100, 450, 450, 450, 400, 450, 400, 450, 450, 450, 1100, 450, 450, 450, 1100, 450, 1150, 450, 400, 450, 450, 450, 1100, 450, 450, 450, 1100, 450, 450, 450, 1100, 450, 450, 450, 400, 450, 400, 450, 450, 450, 400, 450, 450, 450, 400, 450, 400, 500, 400, 450, 1100, 450, 450, 450, 400, 450, 400, 450, 450, 450, 400, 450, 1150, 450, 400, 450, 450, 450, 400, 450, 400, 450, 450, 450, 400, 500, 400, 450, 1100, 450, 450, 450, 1100, 450, 400, 450, 450, 450, 400, 450, 450, 450, 400, 450};
 
 
String readString = String(30);
String statusLed;


void setup(){
  Ethernet.begin(mac, ip);
  
  pinMode(lampada1, OUTPUT);
  pinMode(lampada2, OUTPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(ledIR ,OUTPUT); // saída do infravermelho   
  Serial.begin(9600); 
}

void loop(){
  // Criar uma conexão de cliente
  EthernetClient client = server.available();
  
  if (client) {
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        // ler caractere por caractere vindo do HTTP
        if (readString.length() < 30)
        {
          // armazena os caracteres para string
          readString += (c);
        }
        
        //se o pedido HTTP terminou
        if (c == '\n')
          {
            if (readString.indexOf("lampada1") > 0) {
              digitalWrite(lampada1,!digitalRead(lampada1));
            }
            if (readString.indexOf("lampada2") > 0) {
              digitalWrite(lampada2,!digitalRead(lampada2));
            }
            if (readString.indexOf("ventilador") > 0) {
              digitalWrite(ventilador,!digitalRead(ventilador));
            }
            if (readString.indexOf("wake") > 0) {
                //liga tv
                irsend.sendRC5(0x80C, 32);
                delay(200);
            }
  //--------------------------------------- comeca pagina html 
 
        client.println("HTTP/1.1 200 OK");
	client.println("Content-Type: text/html");
	client.println();
	//comeca o codigo da pagina

        client.println("<META HTTP-EQUIV='refresh' CONTENT='4; URL=//seudominio.no-ip.org/'>");
	client.println("<!doctype html><html><head><title>Automação Residencial</title><meta name='viewport' content='width=320'><meta name='viewport' content='width=device-width'><meta charset='utf-8'><meta name='viewport' content='initial-scale=1.0, user-scalable=no'></head>");
	client.println("<body><center>");
	client.println("<img src='http://www.smarthome-expert.de/wp-content/wp-content/uploads/2014/03/SmartHome-Expert_Final_kleinLogo.jpg' alt='Home Automation' height='250' width='480'><br /> ");
        //botao lampada1
        if(digitalRead(lampada1)){ statusLed = "Ligada"; }  else { statusLed = "Desligada"; }
	client.println("<br /> <form action=/'lampada1' method='get'><button type=submit style='width:400px; height: 80px;'>Lampada 1 -  "+statusLed+"<\/button> </form><br /> ");

        if(digitalRead(lampada2)){ statusLed = "Ligada"; }  else { statusLed = "Desligada"; }
        //botao lampada2
	client.println("<form action='lampada2' method='get'><button type=submit style='width:400px; height: 80px;'>Lampada 2 - "+statusLed+"</button>  </form><br /> ");

        if(digitalRead(ventilador)){ statusLed = "Ligado"; }  else { statusLed = "Desligado"; }
        //botao ventilador
	client.println("<form action='ventilador' method='get'><button type=submit style='width:400px; height: 80px;'>Ventilador - "+statusLed+"</button>  </form><br /> ");

	client.println("</center></body></html>");
 

  //--------------------------------------- final pagina html
        //limpa string para a próxima leitura
        readString="";
        
        // parar cliente
        client.stop();
        }
      }
    }
  }
}

