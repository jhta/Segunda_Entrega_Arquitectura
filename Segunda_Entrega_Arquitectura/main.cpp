#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <sstream>
#include<iostream>
#include<cstdlib>
#include "EasyBMP.h"
#include "stdafx.h"
#include <cstring>
using namespace std;


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//					DEFINICION DE VARIABLES Y FUNCIONES
//WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
//Variables definidas
#define pi_2 (4.0 * asin(1.0))
#define MAX_SIZE 5
#define PI 3.14159
#define T_LOW 65
#define T_HIGH 80
HINSTANCE iniciar;

//Definicion de funciones
int importImg(char*);
void outputImg(void);
void printFileInfo(BMP);
float convolve(int[][5], int,  float, int, int);
void gaussianBlue(void);
void sobel(void);
void noMax(void);
void hysteresis(void);
void hysteresis2(void);
bool isFirstMax(double , double , double );
int getOrientation(float);
bool isBetween(float, float, float, float, float);

//Variables Globales
int T_LOWA;
int T_HIGHA;
int i,j,m,n;
int greaterFounda;
int betweenFounda;
double **imageArray;
double **imageArray2;
double  **thetas;
double  **thetas2;
double** magArray;
double** magArray2;
float doscincuenta;
double estadistica[100];
int INF= (1<<20);

int cont=0;
double sum=0;
double sumASM=0;
double sumC=0;
double miniASM=INF;

double miniC=INF;

unsigned int ROWS;
unsigned int COLUMNS;
char DEPTH;


// Global variables

int N,M;
int R,C;

double dt[1024][1024],Qr[1024][1024],Qi[1024][1024],Qm[1024][1024]; 

// The main window class name.
static TCHAR szWindowClass[] = _T("BMPLoad");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Algoritmo Canny Edge");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//Variables propias para manejo de botones
double   dot[1024][1024],Qra[1024][1024],Qia[1024][1024],Qma[1024][1024];
OPENFILENAME exam ;
wchar_t szFile[MAX_PATH] ;
char *FileNameBufo=(char *)malloc(MAX_PATH);
bool bandera= false; // Esta bandera es un booleano que controla la carga de archivos. En caso de ser falsa nocarga nada, si es verdadera carga un archivo.



/*HWND sHwnd;
 
void SetWindowHandle(HWND hwnd){
    sHwnd= hwnd;
}*/

// **********
// class CRaster
//   - Clase generica para imagenes BMP raster.
void printFileInfo(BMP image){
	  cout << endl << "File info:" << endl;
  cout << image.TellWidth() << " x " << image.TellHeight()
       << " at " << image.TellBitDepth() << " bpp" << endl << endl;
}



//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//					FUNCIONES PARA IMPLEMENTACION DE SOBEL Y CANNY
//WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW

//fUNCION PARA MODIFICAR ARCHIVVO
void outputImg(void){
  //setup Output IMG
  BMP OutputIMG;
  
  int byte;
  OutputIMG.SetBitDepth(DEPTH);
  OutputIMG.SetSize(COLUMNS,ROWS);
  
  cout<< "Output Image to File" << endl;
  for( int j=0 ; j < ROWS ; j++)
    {
      for( int i=0 ; i < COLUMNS ; i++)
	{
	  byte =  imageArray[i][j];
	  OutputIMG(i,j)->Red = byte;
	  OutputIMG(i,j)->Green = byte;
	  OutputIMG(i,j)->Blue = byte;
	}
    }
  
  OutputIMG.WriteToFile("Output.bmp");
  cout << "\n**** NOW GO OPEN Output.BMP ******" << endl;
}
//***************************
// convolve is a general helper funciton that applies a convolution
// to the image and then returns the weighted sum so that
// it can replace whatever pixel we were just analyzing
//**************************
float convolve(int con[][MAX_SIZE], int dim,  float divisor, int i, int j) {
    int midx = dim/2;
    int midy = dim/2;

    float weightedSum = 0;
    for(int x = i-midx; x < i + dim-midx; x++) {
      for(int y = j-midy; y < j + dim-midy; y++) {
	weightedSum += divisor*(double)(con[x-i+midx][y-j+midy]*imageArray[x][y]);
      }
    }
    return weightedSum;
}


//*****************************
// gaussian blur
// applies a gaussian blur via a convolution of a gaussian
// matrix with sigma = 1.4. hard-coded in.
// future development of can generate the gauss matrix on the fly
//*****************************
void gaussianBlur(void) {
  //define gauss matrix
  int gaussArray[5][5] = {  {2, 4, 5, 4, 2},
			  {4, 9, 12,9, 4},
			  {5, 12, 15, 12, 5},
			  {4, 9, 12,9, 4},
			  {2, 4, 5, 4, 2} };

  float gaussDivisor = 1.0/159.0;
  float sum = 0.0;
  int dim = 5;

  for(int i=2; i < COLUMNS-2; i++) {
    for(int j=2; j < ROWS-2; j++) {
      sum = convolve(gaussArray,dim, gaussDivisor, i, j);
      imageArray[i][j] = (int)sum;
    }
  }
}



//****************************
// Applies a sobel filter to find the gradient direction
// and magnitude. those values are then stored in thetas and magArray
// so that info can be used later for further analysis
//****************************
void sobel(void) {
  float sum = 0.0;
  float G_x, G_y, G, theta;
  int sobel_y[5][5] = {  {-1, 0, 1,0,0},
			 {-2, 0, 2,0,0},
			 {-1, 0, 1,0,0}, 
			 {0, 0, 0, 0, 0},
			 {0, 0, 0, 0, 0}};
			  
  int sobel_x[5][5] = {  {1, 2, 1, 0, 0},
			 {0, 0, 0, 0, 0},
			 {-1, -2, -1, 0, 0},
			 {0, 0, 0, 0, 0},
			 {0, 0, 0, 0, 0} };
			
  int dim = 3;
  //columns and rows might be mixed up here
  for (int i = 1; i < COLUMNS-1; i++ ) {
    for (int j = 1; j < ROWS-1; j++ ) {
      G_x = convolve(sobel_x, dim, 1, i, j);
      G_y = convolve(sobel_y, dim, 1, i, j);
      G = sqrt(G_x*G_x + G_y*G_y);
     // cout << G << "\n";
      // cout << atan2(G_y, G_x) << "\n";
      thetas[i][j] = getOrientation(180.0*atan2(G_y, G_x)/PI);
      //	cout << thetas[i][j] << endl;
      magArray[i][j] = G;
    }
  }
}

//***************************
// helper function that returns true if a>b and c
//***************************
bool isFirstMax(double a, double b, double c){
  if(a>b && a>c){
       return true;
  }
  return 0;
}


//****************************
// buckets the thetas into 0, 45, 90, 135
//****************************

int getOrientation(float angle) {
  if(isBetween(angle, -22.5, 22.5, -180, -157.5 ) || isBetween(angle, 157.5, 180, -22.5, 0))
    return 0;
  if(isBetween(angle, 22.5, 67.5, -157.5, -112.5))
    return 45;
  if(isBetween(angle, 67.5, 112.5, -112.5, -67.5))
    return 90;
  if(isBetween(angle, 112.5, 157.5, -67.5, -22.5))
    return 135; 
    
  return -1;

}

//*****************************
//helper function that says whether arg is between a-b or c-d
//*****************************
bool isBetween(float arg, float a, float b, float c, float d) {
  if((arg >= a && arg <= b) || (arg >= c && arg <= d)) {
    return true;
  } else {
    return false;
  }
}


//*****************************
//non-maximum suppression 
//depending on the orientation, pixels are either thrown away or accepted
//by checking it's neighbors
//*****************************
void noMax(void){
	int theta = 0;
	for( int j=1 ; j < ROWS-1 ; j++)
	 {
	    for( int i=1 ; i < COLUMNS-1 ; i++)
	     {
			theta = (int) thetas[i][j];
			switch(theta){
				case 0:
					if(isFirstMax(magArray[i][j],magArray[i+1][j],magArray[i-1][j])){
						imageArray[i][j] = 0; // black
					}
					else{
						imageArray[i][j] = 255; // white
					}
				break;
				
				case 45:
					if(isFirstMax(magArray[i][j],magArray[i+1][j+1],magArray[i-1][j-1])){
						imageArray[i][j] = 0; // black
					}
					else{
						imageArray[i][j] = 255; // white
					}
				
				break;
				
				case 90:
					if(isFirstMax(magArray[i][j],magArray[i][j+1],magArray[i][j-1])){
						imageArray[i][j] = 0.0; // black
					}
					else{
						imageArray[i][j] = 255.0; // white
					}
				break;
				
				case 135:
					if(isFirstMax(magArray[i][j],magArray[i+1][j-1],magArray[i-1][j+1])){
						imageArray[i][j] = 0.0; // black
					}
					else{
						imageArray[i][j] = 255.0; // white
					}
				break;
				
				default:
				  //	cout << "error in nomax()"<< endl;
				break;
			}
		}
	}
}

//*****************************
//non-maximum suppression 
//depending on the orientation, pixels are either thrown away or accepted
//by checking it's neighbors
//*****************************


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//					ALGORITMO EN ENSAMBLADOR DE LA FUNCION NOMAX
//WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
void noMax2(void){
	int theta = 0;
	int i=0, j=0;
	int acumFilas=1, acumColumnas=1, filas=ROWS-1, columnas=COLUMNS-1;
	
	for( int j=1 ; j < ROWS-1 ; j++)
	 {
	    for( int i=1 ; i < COLUMNS-1 ; i++)
	     {
			 __asm{
				 while1:
					mov eax, filas
					mov ebx, acumFilas
					cmp eax, ebx
					jge finwhile
;////////////////////WHILE QUE RECORRE LA MATRIZ
					while2:
						mov eax, columnas
						mov ebx, acumColumnas
						cmp eax, ebx
						jge finwhile
					
					

			 }
			theta = (int) thetas[i][j];
			int bandera1=1;
			int bandera2=1;
			int n_255=255;
			int n_0=0;
			int n_45=45;
			int n_90=90;
			int n_135=135;
			double basura;
		

//////ARRANCA CODIGO CON UN SWITCH, EL CUAL MANEJAMOS CON ETIQUETAS
///////////////////////////////////////////////////
/////////////////////////////////////////////////////
					__asm{

						mov eax, theta
						mov ebx, n_0
						cmp eax, ebx
						je caso0   ;// COMPARA TETHA CON 0 PARA ENTRAR AL PRIMER CASO

						
						mov ebx, n_45
						cmp eax, ebx
						je caso45   ;// COMPARA TETHA CON 45 PARA ENTRAR AL PRIMER CASO


						mov ebx, n_90
						cmp eax, ebx
						je caso90   ;// COMPARA TETHA CON 90 PARA ENTRAR AL PRIMER CASO

						mov ebx, n_135
						cmp eax, ebx
						je caso135   ;// COMPARA TETHA CON 0 PARA ENTRAR AL PRIMER CASO


						caso0:
						finit
					mov esi, i
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j]

					mov esi, i
					inc esi
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i+1][j]


					;/////////COMPARACION
					fcomi ST(0), ST(1)
					jge no_comparacion
					mov bandera1, 0
					fstp basura

					mov esi, i
					dec esi
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i-1][j]

					;//COMPARACION 2
					fcomi ST(0), ST(1)
					jge no_comparacion
					mov bandera2, 0
					fstp basura

					finit
					fldz   ;cargo el cero

					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=0
					
					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					no_comparacion: ;else

					finit
					
					
					fild n_255
					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=255

					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					}
					///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//////eN CASO DE QUE SEA DE 45 GRADOS			
///////////////////////////////////////////////////
/////////////////////////////////////////////////////
					__asm{

						caso45:
						finit
					mov esi, i
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j]

					mov esi, i
					inc esi
					mov edi, j
					inc edi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i+1][j+1]


					;/////////COMPARACION
					fcomi ST(0), ST(1)
					jge no_comparacion2
					mov bandera1, 0
					fstp basura

					mov esi, i
					dec esi
					mov edi, j
					dec esi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i-1][j-1]

					;//COMPARACION 2
					fcomi ST(0), ST(1)
					jge no_comparacion2
					mov bandera2, 0
					fstp basura

					finit
					fldz   ;cargo el cero

					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=0
					
					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					no_comparacion2: ;else

					finit
					
					
					fild n_255
					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=255

					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					}
					///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

					

//////ENN CASO DE QUE SEA DE 90 GRADOS					
///////////////////////////////////////////////////
/////////////////////////////////////////////////////
					__asm{
						caso90:
						finit
					mov esi, i
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j]

					mov esi, i
					
					mov edi, j
					inc edi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j+1]


					;/////////COMPARACION
					fcomi ST(0), ST(1)
					jge no_comparacion3
					mov bandera1, 0
					fstp basura

					mov esi, i
					
					mov edi, j
					dec edi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j-1]

					;//COMPARACION 2
					fcomi ST(0), ST(1)
					jge no_comparacion3
					mov bandera2, 0
					fstp basura

					finit
					fldz   ;cargo el cero

					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=0
					
					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					no_comparacion3: ;else

					finit
					
					
					fild n_255
					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=255

					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					jmp finswitch
					}
					///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

	

//////EN CASO DE QUE SEA DE 135 GRADOS					
///////////////////////////////////////////////////
/////////////////////////////////////////////////////
					__asm{
						caso135:
						finit
					mov esi, i
					mov edi, j
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i][j]

					mov esi, i
					inc esi
					mov edi, j
					dec edi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i+1][j-1]


					;/////////COMPARACION
					fcomi ST(0), ST(1)
					jge no_comparacion4
					mov bandera1, 0
					fstp basura

					mov esi, i
					dec esi
					mov edi, j
					inc edi
					mov edx, type magArray
					imul edi, type double
					imul esi, edx
					fld [qword ptr magArray +esi+edi]  ;Apilo magArray[i-1][j+1]

					;//COMPARACION 2
					fcomi ST(0), ST(1)
					jge no_comparacion4
					mov bandera2, 0
					fstp basura

					finit
					fldz   ;cargo el cero

					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=0
					
					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					no_comparacion4: ;else

					finit
					
					
					fild n_255
					mov esi, i
					mov edi, j
					mov edx, type imageArray
					imul edi, type double
					imul esi, edx
					fstp [qword ptr imageArray +esi+edi]  ;igualo imageArray[i][j]=255

					mov bandera1, 1  ; muevo la bandera a 1 otra vez
					mov bandera2, 1  ; muevo la bandera a 1 otra vez
					
					finswitch:
						mov ebx, acumColumnas
						inc ebx
						mov acumColumnas, ebx

					finwhile2:

						mov eax, acumFilas
						inc eax
						mov acumFilas, eax

				finwhile:
					}
					///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

		}
	}
}

//*******************************
//hysteresis noise filter makes lines continuous and filters out the noise
// see the pdf that we used to understand this step in english (Step 5)
//*******************************
  void hysteresis(void){
    bool greaterFound;
    bool betweenFound;
        for( int j=2 ; j < ROWS-2 ; j++){
	   for( int i=2 ; i < COLUMNS-2 ; i++){
	       if(magArray[i][j] < T_LOW){
		    imageArray[i][j] = 255; // white
	       }
			
	       if(magArray[i][j] > T_HIGH){
		    imageArray[i][j] = 0; // black
	       }
		
	       /*If pixel (x, y) has gradient magnitude between tlow and thigh and 
		 any of its neighbors in a 3 × 3 region around
		 it have gradient magnitudes greater than thigh, keep the edge*/
	       
	       if(magArray[i][j] >= T_LOW && magArray[i][j] <= T_HIGH){
		 greaterFound = false;
		 betweenFound = false;
		 for(int m = -1; m < 2; m++) {
		   for(int n = -1; n < 2; n++){
		     if(magArray[i+m][j+n] > T_HIGH) { 
		       imageArray[i][j] = 0;
		       greaterFound = true;
		     }	     
		     if(magArray[i][j] > T_LOW && magArray[i][j] < T_HIGH) betweenFound = true;
		   }
		 }
		 
		 if(!greaterFound && betweenFound) {
		   for(int m = -2; m < 3; m++) {
		     for(int n = -2; n < 3; n++) {
		       if(magArray[i+m][j+n] > T_HIGH) greaterFound = true;
		     }
		   }
		 }
		 
		 if(greaterFound) imageArray[i][j] = 0;
		 else imageArray[i][j] = 255;
		 
	       }
	       
	     }
	 }
  }	


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//					FUNCION HYSTERESIS EN ENSAMBLADOR
//WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW

  void hysteresis2(void){
	   doscincuenta=255.0; //constante utilizada parda realizar los procedimientos
	  T_LOWA=65;   //asignacion inicial de variables
	  cout<<"ROWS-2 // "<<ROWS-2<<endl;  //impresiones para verificar funcionamiento de la aplicación
	  cout<<"COLUMNS-2 // "<<COLUMNS-2<<endl; //impresiones para verificar funcionamiento de la aplicación
	  T_HIGHA=80;  //asignacion inicial de variables
	  _asm{
		  mov j,2 ;//se inicia la j en 2, el for comienza desde 2 y termina en ROWS-2
while1:   ;// los for de esta funcion fueron implementados como whiles,esto debido a lo grande que son y la cantidad de operaciones que realizan
	  }
	  cout<<"j "<<j<<endl;  //impresiones de prueba
	  _asm{
	mov eax,j
	mov ebx,ROWS
	sub ebx,2
	cmp eax,ebx    ;//aca se encuentran las comparaciones para verificar si se debe salir del while o no
	jge finwhile1   ; //salta si cumple la co
	mov i,2 ;//la i comienza en 2, el for va desde 2 hata COLUMNS-2
while2:  ;//este for tambien fue implementando como un while
	  }
	  cout<<"i "<<i<<endl;  // impresiones de verificacion
	  _asm{
		mov eax,i
		mov ebx,COLUMNS
		sub ebx,2
		cmp eax,ebx   ; //comparaciones para verificar si se debe terminar el procedimiento o no
		
		jge finwhile2
		
		
			mov esi,i
		mov edi,j
		mov edx,type magArray
		imul edi,type double ;//multiplicar edi por el tipo del dato del arreglo
		imul esi,edx  ;// manejo de indices para poder acceder a ciertas posiciones de la matriz
		;fild T_LOWA
		;fld [qword ptr magArray +esi+edi]
		;fcomi ST(0),ST(1)
		;jge finsi1                   ;//estas lineas verifican ciertas condiciones para poder cambiar
		;finit                        ;//ciertas posiciones de la matriz imageArray por el numero 255
		;fld doscincuenta 
		;fstp [qword ptr magArray +esi+edi]
		;finsi1:
		
	  }
	  if(magArray[i][j]<T_LOW){
		  imageArray[i][j]=255;   //lineas de apoyo en c++ para verficiar condiciones
	  }
	  if(magArray[i][j]>T_HIGH){  //lineas de apoyo en c++ para verificar condiciones
		  imageArray[i][j]=0;
	  }
	  _asm{
		
		fild T_LOWA
		fld [qword ptr magArray +esi+edi]
		fcomi ST(0),ST(1)
		jl finsi3
		finit
		fild T_HIGHA
		fld [qword ptr magArray +esi+edi]
		fcomi ST(0),ST(1)
		jg finsi3
		mov greaterFounda,0   ; //la unica forma de entrar a realizar los siguientes procedimientos
		mov betweenFounda,0 ;// es que cumpla estas condiciones if(magArray[i][j] >= T_LOW && magArray[i][j] <= T_HIGH){
		mov eax,0  
		sub eax,1  ;//esta línea marca el inicio de la variable m, la cual comienza en -1 y se utiliza en el siguiente while
		mov m,eax  ; 
		while3:
		mov eax,m
		mov ebx,2
		cmp eax,ebx   ;//comparaciones para verificar y controlar la ejecucion del while, para que no se vaya a un cico infinito
		jge finwhile3
		
		mov eax,0  
		sub eax,1
		mov n,eax  ; //inicializacion de la variable n , comienza en -1
		while4:
		mov eax,n
		mov ebx,2
		cmp eax,ebx  ;//comparaciones de control del ciclo while, para que no se vaya a un ciclo infinito
		jge finwhile4
		
		mov eax,i
		add eax,m
		mov esi,eax
		mov edx,type magArray ; //manejo de los indices de la matriz
		imul esi,edx   ;// manejo de los indices de la matriz
		mov eax,j
		add eax,n
		mov edi,eax
		imul edi,type double
		finit
		fild T_HIGHA
		fld [qword ptr magArray +esi+edi]
		fcomi ST(0),ST(1) ; //realiza la siguiente comparacion if(magArray[i+m][j+n] > T_HIGH) {
		jle finsi4
		mov esi,i
		mov edi,j
		mov edx,type imageArray
		imul edi,type double //multiplicar edi por el tipo del dato del arreglo
		imul esi,edx
		finit
		fldz
		fstp [qword ptr imageArray +esi+edi] ;// realiza la siguiente asignacion imageArray[i][j] = 0;
		mov greaterFounda,1 ;//la variable greaterFounda queda en True, equivalente a quedar en 1
		finsi4:
		
		
		mov esi,i
		mov edi,j
		mov edx,type magArray
		imul edi,type double //multiplicar edi por el tipo del dato del arreglo
		imul esi,edx
		fild T_LOWA
		fld [qword ptr magArray +esi+edi]
		fcomi ST(0),ST(1)
		jle finsi5
		 la variable m en 1, esto para el manejo del ciclo while
		jmp while3
		finwhile3:
	
		mov eax,0
		cmp eax,greaterFounda
		jl finsi6
		mov eax,1
		cmp eax,betweenFounda   ;//este bloque de codigo realiza las siguientes comparaciones if(!greaterFound && betweenFound) {
		jg finsi6

		mov eax,0    ;// a continuacion, si cumplio con las condiciones anteriores procedera a entrar en un doble ciclo while
		sub eax,2finit
		fild T_HIGHA
		fld [qword ptr magArray +esi+edi]  ;// este bloque de codigo realiza la siguiente comparacion if(magArray[i][j] > T_LOW && magArray[i][j] < T_HIGH)
		fcomi ST(0),ST(1)
		jge finsi5
		mov betweenFounda,1 ;//asigna a betweenFounda un uno, equivalente a asignarle un True como en el caso del algoritmo de C++
		
		finsi5:
		
		inc n  ;//incremento de variable n en 1, esto para el manejo del ciclo while
		jmp while4
		finwhile4:
		inc m ://incremento de
		mov m,eax  ; //asgina un valor inicial a m para así poder iniciar con el ciclo while
		while5:
		mov eax,m
		mov ebx,3
		cmp eax,ebx ;//comparaciones para controlar la ejecucion del ciclo while, se realiza hasta que sea mayor o igual a 3
		jge finwhile5

		mov eax,0  
		sub eax,2
		mov n,eax  ; //asginacion inicial de la variable n para la ejecucion del ciclo while6
		while6:
		mov eax,n
		mov ebx,3
		cmp eax,ebx ;//comparaciones para control del ciclo while
		jge finwhile6
		
		mov eax,i
		add eax,m
		mov esi,eax
		mov edx,type magArray ;//manejo de indices de la matriz
		imul esi,edx ;//manejo de indices de la matriz
		mov eax,j
		add eax,n
		mov edi,eax
		imul edi,type double ;//manejo de indices de la matriz
		finit
		fild T_HIGHA
		fld [qword ptr magArray +esi+edi]
		fcomi ST(0),ST(1) ;// esto realiza la siguiente comparacion if(magArray[i+m][j+n] > T_HIGH)
		jle finsi7
		mov greaterFounda,1 ;//asigna a esta variable un 1 si cumplió con la condicion anterior, equivalente en c++ a asignar True
		finsi7:
		inc n ;// incrementa n para poder continuar con el flujo del ciclo
		jmp while6
		finwhile6:
		inc m ;//incrementa a m para continuar con el flujo del ciclo
		jmp while5
		finwhile5:
		finsi6:
		finsi3:
		inc i ;//incremento de la variable i para continuar con el ciclo while
		jmp while2
	finwhile2:
	inc j ;//incremento de la variable j para continuar con el ciclo while mas externo
	  }
	 // cout<<"i "<<i<<endl; //impresiones para verificacion y depuracion del algoritmo
	  _asm{
	jmp while1
finwhile1: ;//fin el while mas externo, cuando termine termina la ejecucion de esta función
	  }
  }
class CRaster {
	public:
		int Width,Height;		// Dimensiones
		int BPP;				// Bits por Pixel.
		char * Raster;			// Bits de la Imagen.
		RGBQUAD * Palette;		// Paleta RGB para la imagen.
		int BytesPerRow;		// Ancho de las filas (en bytes).
		BITMAPINFO * pbmi;		// estructura BITMAPINFO

		// Funciones miembro de la clase (se definen despues):
		int LoadBMP (char * szFile);
		int GDIPaint (HDC hdc,int x,int y, int w, int h);
		void AbrirArchivo(HWND hWnd);
};

// **********
// Función principal de Windows. 
//   - Inicio del programa


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(RGB(229,203,255));
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Fallo el registro de la ventana!"),
            _T("Win32 Guided Tour"),
            NULL);

        return 1;
    }

    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 700,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Win32"),
            NULL);

        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,SW_MAXIMIZE);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//					FUNCION DONDE SE INICIA Y SE REALIZA EL PROCESO DE DIBUJO
//WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    //variables para botones y barra de menu
    int winMenuId, winMenuEvent;
    static HWND examinar, estadisticas, Instruc, Salir;
    RECT rect;
    	static CRaster bmp;

	LPCWSTR a;

	long cl;
	int x,y,f,g,R,G,B, FilaColumnaQr,FilaColumnaQi,FilaColumnaQm,FilaColumnaDt,columnasfg,columnasxy; // Definiciión de variables necesarias para el cálculo de la trasformada
	int _2_l = 2;
	double ang;
	ofstream ar_r,ar_i,ar_m;
	double r_m_n;
	char mensaje[50];
	clock_t tc,ta; // Se crean dos variables para tomar el tiempo del cálculo en c++ (tc) y en assembley (ta)
	double TiempoAssembly, TiempoC;
	wstringstream TIEMPOS;

    TCHAR greeting[] = _T("Algoritmo Canny Edge");
	//vARIABLES PARA IMPRIMIR LABELS
	static HWND hwndDireccion ;
	static HWND hwndBienvenida ;
	//static HWND iniciar ;
    switch (message)
    {
	case WM_CREATE:

		//MENSAJE DE INSTRUCCIONES
		hwndDireccion = CreateWindow(L"EDIT", L"		INSTRUCCIONES: ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 50, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L"____________________________________________________________", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 70, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" La función del presente programa es por medio del algoritmo", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 90, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" de Canny Edge  es detectar  los  bordes  de  alguna  imagen", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 110, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" seleccionada.", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 130, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 150, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" Para seleccionar una imagen, se  debe  presionar  el  botón", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 170, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" Examinar, inmediatamente aparecerá la venta para seleccionar", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 190, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" la imagen a transformar, teniendo esto el programa procederá", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 210, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" a realizar los cálculos en C++ y  en  Assambler,  calculando", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 230, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" también sus  respectivos  tiempos.  Luego  se  mostrara  por", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 250, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" pantalla las imágenes respectivas.", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 270, 410, 20, hWnd, 0, iniciar, NULL);
		hwndDireccion = CreateWindow(L"EDIT", L" ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 290, 410, 20, hWnd, 0, iniciar, NULL);
		
		
		
		//MENSAJE DE INFORMACION
		hwndBienvenida = CreateWindow(L"EDIT", L"  DETECTOR DE BORDES CON ALGORITMO DE CANNY EDGE  ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 350, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L"__________________________________________________", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 370, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Fecha de creación:  16/12/2013", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 390, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Versión 1.0.0.0", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 410, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Desarrolladores:", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 430, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" °Jeison Higuita Sanchez", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 450, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" °Jonathan Vallejo Muñoz", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 470, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 490, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Universidad Nacional de Colombia", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 510, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Arquitectura de Computadores", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 530, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" Medellín", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 550, 410, 20, hWnd, 0, iniciar, NULL);
		hwndBienvenida = CreateWindow(L"EDIT", L" ", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 900, 570, 410, 20, hWnd, 0, iniciar, NULL);
		

		//Se crea un botón examinar
		examinar = CreateWindow(L"BUTTON", L"Examinar", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								260,300, 100,38, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		/// se crea el botón salir, para salir del programa
		Salir = CreateWindow(L"BUTTON", L"Salir", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								400,300, 100,38, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		estadisticas = CreateWindow(L"BUTTON", L"Estadisticas", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								540,300, 100,38, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		
		break;
	case WM_COMMAND:
		if(examinar == (HWND)lParam){ //SI PRESIONA EL BOTON DE EXAMINAR

			bmp.AbrirArchivo(hWnd);
			if(bandera){
				bandera= false;
				
				GetClientRect(hWnd,&rect);
				Rectangle((HDC)wParam,rect.left,rect.top,rect.right,rect.bottom);
				InvalidateRect(hWnd,NULL,TRUE);
				// Inicia un dispositivo de dibujo
				hdc=BeginPaint (hWnd,&ps);
				// Pega la imagen en el dispositivo de dibujo en las coordenadas 0,0
				bmp.GDIPaint (hdc,130,70,0,0);
				// Se recorre la imagen y se lee el color del punto, si es negro (0 = RGB(0,0,0))
				// Se pinta con rojo (1000), sino, se deja intacto
				ROWS=bmp.Height; // Altura de la Imagen
				COLUMNS=bmp.Width; // Ancho de la Imagen

				//allocate Memory
				imageArray = new double *[COLUMNS] ; // row memory allocation
				for( int i = 0 ; i < COLUMNS ; i++ ){ // column memory allocation
				imageArray[i] = new double[ROWS];
				}
				imageArray2 = new double *[COLUMNS] ; // row memory allocation
				for( int i = 0 ; i < COLUMNS ; i++ ){ // column memory allocation
				imageArray2[i] = new double[ROWS];
				}

				thetas = new double *[COLUMNS];
				for(int i = 0; i < COLUMNS; i++) {
				thetas[i] = new double[ROWS];
				}

				
				magArray = new double *[COLUMNS];
				for(int i=0; i < COLUMNS; i++) {
				magArray[i] = new double[ROWS];
				}
				
				int Temp;
				//r_m_n = sqrt((double)(N*M));	// Valor necesario para cálculo de la transformada (Raiz cuadrada del área de la imagen).
				for (f=0;f<ROWS;f++) // Primer ciclo para llenado de las Matrices dt y dot
				{
					for (g=0;g<COLUMNS;g++) // Segundo ciclo para llenado de las Matrices dt y dot
					{
						cl=GetPixel(hdc,g+130,f+70);	// Función para obtener el pixel en la posición dada
						B=GetBValue(cl);			// Valor o cantidad de azul en el pixel
						R=GetRValue(cl);			// Valor o cantidad de rojo en el pixel
						G=GetGValue(cl);			// Valor o cantidad de verde en el pixel
						Temp=(int)(((0.299*(float)R+0.587*(float)G+0.114*(float)B)));		// Cálculo del valor en cada posición de la matriz dt
						//dot[f][g]=(float)(((0.3*(float)R+0.59*(float)G+0.11*(float)B))*(float)(pow(-1.0,(double)(f+g))));		// Cálculo del valor en cada posición de la matriz dot (para uso en assembler)
						imageArray[g][f] = Temp;
						imageArray2[g][f] = Temp;
					}
				}
///////MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
				//CALCULO EN C++
///////WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
				tc= clock(); // Se inicia a tomar el tiempo que demora haciendo el cálculo
				gaussianBlur();
				sobel();
				noMax();
				hysteresis();
				
				tc= clock() - tc; // Se toma el tiempo y se le resta el que tenía.
				TiempoC= ((double)tc / ((double)(CLOCKS_PER_SEC)))*1000; // Se pasa Tiempo C++ a milisegundos
				SetTextColor(hdc,RGB(0,0,0)); // Color para la letra
				SetBkColor(hdc,RGB(229,203,255));		// Color para el fondo del label de la letra
				TextOutA(hdc,M+340,(50),"Canny Edge en C++: ",20); // Texto para mostrar la transformada de Fourier en c++
	
					for (f=0;f<ROWS;f++)
				{
					for (g=0;g<COLUMNS;g++)
					{//PINTA LA IMAGEN CALCULADA CON C++
						SetPixel(hdc,f+M+340,g+N+70,RGB((int)imageArray[f][g],(int)imageArray[f][g],(int)imageArray[f][g]));
					}
				}
				

				//outputImg();
				imageArray = new double *[COLUMNS] ; // row memory allocation
				for( int i = 0 ; i < COLUMNS ; i++ ){ // column memory allocation
				imageArray[i] = new double[ROWS];
				}
				thetas = new double *[COLUMNS];
				for(int i = 0; i < COLUMNS; i++) {
				thetas[i] = new double[ROWS];
				}
				magArray = new double *[COLUMNS];
				for(int i=0; i < COLUMNS; i++) {
				magArray[i] = new double[ROWS];
				}
				for (f=0;f<ROWS;f++) // Primer ciclo para llenado de las Matrices dt y dot
				{
					for (g=0;g<COLUMNS;g++) // Segundo ciclo para llenado de las Matrices dt y dot
					{
						
						imageArray[g][f] = imageArray2[g][f];
						
					}
				}


	
///////MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
				//CALCULO EN ASSAMBLER
///////WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
				
				ta= clock(); // Se incia a tomar el tiempo que demora en hacer el cálculo.
				gaussianBlur();
				sobel();
				noMax2();
				hysteresis2();
				ta= clock() - ta; // Se hace la resta del tiempo en ese preciso momento y el que se tenía de referencia.
				TiempoAssembly= ((double)ta / ((double)CLOCKS_PER_SEC)) * 1000; // Se pasa TiempoASM a milisegundos

				SetTextColor(hdc,RGB(0,0,0)); // Color para la letra
				SetBkColor(hdc,RGB(229,203,255));		// Color para el fondo del label de la letra
				TextOutA(hdc,M+550,(N+50),"Canny Edge en ASM: ",20); // Texto para mostrar la transformada de Fourier en asm
			
					for (f=0;f<ROWS;f++)
				{
					for (g=0;g<COLUMNS;g++)
					{//PINTA LA IMAGEN CALCULADA CON ASSAMBLY
						SetPixel(hdc,f+M+550,g+N+70,RGB((int)imageArray[f][g],(int)imageArray[f][g],(int)imageArray[f][g]));
					}
				}

				// Se llena el buffer para mostrar por un messageBox
				TIEMPOS<<"Tiempo C++: "<<TiempoC<<" milisegundos"<<endl
					<<"Tiempo ASM: "<<TiempoAssembly<<" milisegundos"<<endl
					<<"Diferencia: "<<abs(TiempoC-TiempoAssembly)<<" milisegundos";
				sum+=abs(TiempoC-TiempoAssembly);
				sumASM+=TiempoAssembly;
				sumC+=TiempoC;
				cont++;
				miniASM=min(miniASM,TiempoAssembly);
				miniC=min(miniC,TiempoC);
				//MUESTRA LOS TIEMPOS
				MessageBox(NULL,TIEMPOS.str().c_str(),_T("Tiempos"),NULL);
				
				



				EndPaint(hWnd, &ps);
		}
		}else if(Salir == (HWND)lParam){
			DestroyWindow(hWnd);
		}
		else if(estadisticas == (HWND)lParam){
			if(cont==0){
				wstringstream IMPRIMIR;
				IMPRIMIR<<"Por el momento no se a ingresado ningun dato "<<endl;
				MessageBox(NULL,IMPRIMIR.str().c_str(),_T("Estadisticas"),NULL);
			}else{
				double res=sum/((double)cont);
				wstringstream IMPRIMIR;
				IMPRIMIR<<"Promedio de diferencias: "<<res<<" milisegundos"<<endl
					<<"Promedio en ASM: "<<(sumASM/((double)cont ))<<" milisegundos"<<endl
					<<"Promedio en C++: "<<(sumC/((double)cont ))<<" milisegundos"<<endl
					<<"Tiempo ASM minimo: "<<miniASM<<" milisegundos"<<endl
					<<"Tiempo C++ minimo: "<<miniC<<" milisegundos"<<endl;
					

						MessageBox(NULL,IMPRIMIR.str().c_str(),_T("Estadisticas"),NULL);
			


			}
		}
	    break; 
    case WM_PAINT:
		hdc=BeginPaint (hWnd,&ps);
		EndPaint(hWnd, &ps);
	   break;
    
    case WM_DESTROY:
                MessageBox(NULL,
            _T("El progama se a ejecutado con exito"),
            _T("Fin"),
            NULL);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}


// **********
// CRaster::LoadBMPFile (FileName);
//   - Carga un archivo BMP en un objeto CRaster
//   * soporta archivos non-RLE-compressed de 1, 2, 4, 8 & 24 bits-por-pixel
int CRaster::LoadBMP (char * szFile)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	// Abrir archivo.
	ifstream bmpfile (szFile , ios::in | ios::binary);
	if (! bmpfile.is_open()) return 1;		// Error al leer el archivo

	// Cargar bitmap fileheader & infoheader
	bmpfile.read ((char*)&bmfh,sizeof (BITMAPFILEHEADER));
	bmpfile.read ((char*)&bmih,sizeof (BITMAPINFOHEADER));

	// Comprobar el tipo de archivo
	if (bmfh.bfType!='MB') return 2;		// El archivo no es un BMP

	// Asignación de algunas variables necesarias:
	BPP=bmih.biBitCount;  //Bits por pixel
	Width=bmih.biWidth;   //Ancho de la imagen
	Height= (bmih.biHeight>0) ? bmih.biHeight : -bmih.biHeight; // Valor absoluto de la altura
	BytesPerRow = Width * BPP / 8;
	BytesPerRow += (4-BytesPerRow%4) % 4;	// Byts por fila

	// Si BPP no es 24, Carga la paleta
	if (BPP==24) pbmi=(BITMAPINFO*)new char [sizeof(BITMAPINFO)];
	else
	{
		pbmi=(BITMAPINFO*) new char[sizeof(BITMAPINFOHEADER)+(1<<BPP)*sizeof(RGBQUAD)];
		Palette=(RGBQUAD*)((char*)pbmi+sizeof(BITMAPINFOHEADER));
		bmpfile.read ((char*)Palette,sizeof (RGBQUAD) * (1<<BPP));
	}
	pbmi->bmiHeader=bmih;

	// Cargar Raster
	bmpfile.seekg (bmfh.bfOffBits,ios::beg);

	Raster= new char[BytesPerRow*Height];

	// (Si la altura es positiva el BMP esta grabado de abajo-arriba, se lee en sentido inverso)
	if (bmih.biHeight>0)
		for (int n=Height-1;n>=0;n--)
			bmpfile.read (Raster+BytesPerRow*n,BytesPerRow);
	else
		bmpfile.read (Raster,BytesPerRow*Height);

	// Una dirección arriba abajo es negativa, se ajusta:
	pbmi->bmiHeader.biHeight=-Height;

	bmpfile.close();

	return 0;
}

// **********
// CRaster::GDIPaint (hdc,x,y);
// * Pintar el raster en el Dispositivo de Contexto de la Ventana (DC).
int CRaster::GDIPaint (HDC hdc,int x,int y, int w, int h)
{
	// Pegar la imagen en el DC.
	return SetDIBitsToDevice (hdc,x,y,Width,Height,w,h,0,Height,(LPVOID)Raster,pbmi,0);
}

void CRaster::AbrirArchivo(HWND hWnd){
		ZeroMemory( &exam , sizeof( exam));
		exam.lStructSize = sizeof ( exam );
		exam.hwndOwner = hWnd;
		exam.lpstrFile = szFile;
		exam.lpstrFile[0] = '\0';
		exam.nMaxFile = sizeof( szFile );
		exam.lpstrFilter = _TEXT("Bitmap\0*.BMP\0");
		exam.nFilterIndex =1;
		exam.lpstrFileTitle = NULL ;
		exam.nMaxFileTitle = 0 ;
		exam.lpstrInitialDir=NULL ;
		exam.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;

		if(GetOpenFileName(&exam)){
			i= wcstombs(FileNameBufo,exam.lpstrFile,MAX_PATH);	// Se pasa el tipo de dato de exam.lpstrFile a char * y se guarda en FileNameBufo
			bandera= true;
			LoadBMP(FileNameBufo);
		}
}
