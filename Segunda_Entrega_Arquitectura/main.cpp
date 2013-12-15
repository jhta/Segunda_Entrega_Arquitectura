
#include "EasyBMP.h"
#include "stdafx.h"
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

using namespace std;


#define pi_2 (4.0 * asin(1.0))


//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//VARIBLES UTILIZADAS 

#define MAX_SIZE 5
#define PI 3.14159
#define T_LOW 65
#define T_HIGH 80


int **imageArray;
float  **thetas;
int** magArray;

unsigned int ROWS;
unsigned int COLUMNS;
char DEPTH;
int N,M;
int R,C;

double dt[1024][1024],Qr[1024][1024],Qi[1024][1024],Qm[1024][1024]; 


//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//FUNCIONES UTILIZADAS

int importImg(char*);
void outputImg(void);
void printFileInfo(BMP);
float convolve(int[][5], int,  float, int, int);
void gaussianBlue(void);
void sobel(void);
void noMax(void);
void hysteresis(void);
bool isFirstMax(int , int , int );
int getOrientation(float);
bool isBetween(float, float, float, float, float);

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// The main window class name.
static TCHAR szWindowClass[] = _T("BMPLoad");

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Transformada Bidimensional Discreta de Fourier");

HINSTANCE hInst;

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//Variables propias para manejo de botones
double   dot[1024][1024],Qra[1024][1024],Qia[1024][1024],Qma[1024][1024];
OPENFILENAME exam ;
wchar_t szFile[MAX_PATH] ;
char *FileNameBufo=(char *)malloc(MAX_PATH);
int i; 
bool bandera= false; // Esta bandera es un booleano que controla la carga de archivos. En caso de ser falsa nocarga nada, si es verdadera carga un archivo.

//*************************************************************************************************************************************
//*************************************************************************************************************************************
//*************************************************************************************************************************************
//*********************************************DEFINICION DE FUNCIONES******************************************************************



//*****************************
// prints size of img and depth of bits
//*****************************/

void printFileInfo(BMP image){
  cout << endl << "File info:" << endl;
  cout << image.TellWidth() << " x " << image.TellHeight()
       << " at " << image.TellBitDepth() << " bpp" << endl << endl;
}


//***************************** 
// uses the EasyBMP libary to import a bmp image
// and then takes the RGB and calulates a brightness
// assigning it to another array that we can use for 
// further manipulation
// The function essentially loads the file of the name given
// and then desaturates it
//*****************************

int importImg(char* filename){
  BMP InputIMG;
  cout << "Starting BMP final code" << endl;
  cout << "Open File: " << filename << endl;
  if(!InputIMG.ReadFromFile(filename)){
    cout << "Invalid File Name..." << endl;
    return EXIT_FAILURE;
  }

  printFileInfo(InputIMG);
  COLUMNS = InputIMG.TellWidth();  // num cols
  ROWS = InputIMG.TellHeight(); // num rows
  DEPTH = InputIMG.TellBitDepth();
  
  //allocate Memory
  imageArray = new int *[COLUMNS] ; // row memory allocation
  for( int i = 0 ; i < COLUMNS ; i++ ){ // column memory allocation
    imageArray[i] = new int[ROWS];
  }

  thetas = new float *[COLUMNS];
  for(int i = 0; i < COLUMNS; i++) {
    thetas[i] = new float[ROWS];
  }

  magArray = new int *[COLUMNS];
  for(int i=0; i < COLUMNS; i++) {
    magArray[i] = new int[ROWS];
  }

  int Temp;
  cout<< "Saving Brightness values" << endl;
  for( int j=0 ; j < ROWS ; j++)
    {
      for( int i=0 ; i < COLUMNS ; i++)
	{
	  Temp = (int) floor( 0.299*InputIMG(i,j)->Red +
			      0.587*InputIMG(i,j)->Green +
			      0.114*InputIMG(i,j)->Blue );
	  imageArray[i][j] = Temp;
	}
    }

}


//*******************
// outputs the image to a bmp
// by writing the value of the array we have 
// to each of RG, and B
//*******************
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
bool isFirstMax(int a, int b, int c){
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
	/*
	int j=1,i=1;
	float auxMatriz[ROWS*COLUMNS];
	for (int k=1, k<ROWS-1;k++){
		for(int p=1; p<COLUMNS-1;p++){
			auxMatriz[(i*j)+j]=(float)thetas[i][j];
		}
	
	}


	int guardaEcx=0;
	int guardaEsi=0;

	//fist para retornar un entetro
	

	__asm{
	mov eax, ROWS
	sub eax,1
	mov ecx,eax
	mov esi,1

	for1:
		mov guardaEcx, ecx
		mov guardaEsi, esi
;****asigno valor a a clumna
		mov eax, COLUMNS
		sub eax,1
		mov ecx,eax
		mov esi,1

		for2:
			
			fld auxMatriz
			fist theta


			
			inc esi
		loop for2

		inc esi
	loop for1


    }*/

	
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
						imageArray[i][j] = 0; // black
					}
					else{
						imageArray[i][j] = 255; // white
					}
				break;
				
				case 135:
					if(isFirstMax(magArray[i][j],magArray[i+1][j-1],magArray[i-1][j+1])){
						imageArray[i][j] = 0; // black
					}
					else{
						imageArray[i][j] = 255; // white
					}
				break;
				
				default:
				  //	cout << "error in nomax()"<< endl;
				break;
			}
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

//creo un auxiliar para la matriz
	float auxMatriz[ROWS*COLUMNS];
	for (int k=1, k<ROWS-1;k++){
		for(int p=1; p<COLUMNS-1;p++){
			auxMatriz[(p*k)+k]=(float)magArray[p][k];
		}
	
	}

	/*
	int guardaEcx=0;
	int guardaEsi=0;
	int valorEsi=0;

	//fist para retornar un entetro
	

	
	__asm{
	mov eax, ROWS
	sub eax,2
	mov ecx,eax
	mov esi,2

	for1:
		mov guardaEcx, ecx
		mov guardaEsi, esi
;****asigno valor a a clumna
		mov eax, COLUMNS
		sub eax,2
		mov ecx,eax
		mov esi,2
		mov valorEsi, esi
		for2:
			
			fcomi 
			
			inc esi
		loop for2

		inc esi
	loop for1


    }*/



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




/*HWND sHwnd;
 
void SetWindowHandle(HWND hwnd){
    sHwnd= hwnd;
}*/

// **********
// class CRaster
//   - Clase generica para imagenes BMP raster.
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
    wcex.hbrBackground  = CreateSolidBrush(RGB(95,188,211));
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

  /*							//CONSOLA DE WINDOWS, BORRAR ANTES DE FINALIZAR EL TRABAJO 
  //Borrar LUEGO
     AllocConsole() ;
  AttachConsole( GetCurrentProcessId() ) ;
  freopen( "CON", "w", stdout ) ;
  printf("HELLO!!! I AM THE CONSOLE!" ) ;*/

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Fallo el registro de la ventana!"),
            _T("Win32 Guided Tour"),
            NULL);

        return 1;
    }

    hInst = hInstance; // Store instance handle in our global variable

    // The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    //variables para botones y barra de menu
    int winMenuId, winMenuEvent;
    static HWND examinar, AcercaDe, Instruc, Salir;
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

    TCHAR greeting[] = _T("TRANSFORMADA DISCRETA DE FOURIER!");

    switch (message)
    {
	case WM_CREATE:
		// Debe insertar la función para buscar archivos (Filtro BMP)
		// y usarlo como parametro en la función siguiente, como ejemplo pongo
		// la imagen xy.bmp
		
		//Se crea un botón examinar
		examinar = CreateWindow(L"BUTTON", L"Examinar", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								20,50, 90,25, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		//Se crea un botón instrucciones para mostrar al usuariio como usar el programa
		Instruc = CreateWindow(L"BUTTON", L"Instrucciones", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								20,20, 100,25, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		// Se crea botón About, para dar información respecto al programa 
		AcercaDe = CreateWindow(L"BUTTON", L"Acerca de", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								20,80, 90,25, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		// se crea el botón salir, para salir del programa
		Salir = CreateWindow(L"BUTTON", L"Salir", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON|BS_MULTILINE,  
								20,110, 90,25, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
		break;
	case WM_COMMAND:
		if(examinar == (HWND)lParam){
	

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////AQUI COMENIZA LA FUNCION DEL BOTON EXAMINAR//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			bmp.AbrirArchivo(hWnd);
			if(bandera){
				bandera= false;
				MessageBox(hWnd,L"Sea paciente mientras se realizan los cálculos.", L"Please Wait",MB_OK+MB_ICONINFORMATION);

				GetClientRect(hWnd,&rect);
				Rectangle((HDC)wParam,rect.left,rect.top,rect.right,rect.bottom);
				InvalidateRect(hWnd,NULL,TRUE);
				// Inicia un dispositivo de dibujo
				hdc=BeginPaint (hWnd,&ps);
				// Pega la imagen en el dispositivo de dibujo en las coordenadas 0,0
				bmp.GDIPaint (hdc,130,20,0,0);
				// Se recorre la imagen y se lee el color del punto, si es negro (0 = RGB(0,0,0))
				// Se pinta con rojo (1000), sino, se deja intacto
				
				///////////////AQUI ESTOY CHIMBIANDO
				/*
				M=bmp.Height; // Altura de la Imagen
				N=bmp.Width; // Ancho de la Imagen
				
				r_m_n = sqrt((double)(N*M));	// Valor necesario para cálculo de la transformada (Raiz cuadrada del área de la imagen).
				for (f=0;f<M;f++) // Primer ciclo para llenado de las Matrices dt y dot
				{
					for (g=0;g<N;g++) // Segundo ciclo para llenado de las Matrices dt y dot
					{
						cl=GetPixel(hdc,g+130,f+20);	// Función para obtener el pixel en la posición dada
						B=GetBValue(cl);			// Valor o cantidad de azul en el pixel
						R=GetRValue(cl);			// Valor o cantidad de rojo en el pixel
						G=GetGValue(cl);			// Valor o cantidad de verde en el pixel
						dt[f][g]=(float)(((0.3*(float)R+0.59*(float)G+0.11*(float)B))*(float)(pow(-1.0,(double)(f+g))));		// Cálculo del valor en cada posición de la matriz dt
						dot[f][g]=(float)(((0.3*(float)R+0.59*(float)G+0.11*(float)B))*(float)(pow(-1.0,(double)(f+g))));		// Cálculo del valor en cada posición de la matriz dot (para uso en assembler)
					}
				}

				tc= clock(); // Se inicia a tomar el tiempo que demora haciendo el cálculo
		
				for (f=0;f<M;f++) // Primer Ciclo para manejo de las matrices Qr(real), Qi(imaginaria), Qm(módulo)
				{
					for (g=0;g<N;g++) // Segundo Ciclo para manejo de las matrices Qr(real), Qi(imaginaria), Qm(módulo)
					{
						Qr[f][g]=0.0; // Se llena la matriz de reales en ceros
						Qi[f][g]=0.0; // Se llena la segunda matriz de reales en ceros
						for (x=0;x<M;x++) // Tercer ciclo para cálculo de los valores en cada posición de las matrices mencionadas
						{
							for (y=0;y<N;y++) // Cuarto ciclo para cálculo de los valores en cada posición de las matrices mencionadas
							{
								ang = pi_2*((float)(f*x)/(float)M+(float)(g*y)/(float)N);	// Cálculo del ángulo
						
								Qr[f][g]+=dt[x][y]*cos(ang); // Llenado del valor en las posiciones de la matriz real, se usa el dt cuyos valores en las mismas posiciones ya fueron hallados
								Qi[f][g]-=dt[x][y]*sin(ang);	// Llenado del valor en las posiciones de la matriz Imaginaria, se usa el dt cuyos valores en las mismas posiciones ya fueron hallados					
							}
					
						}

						Qr[f][g]=Qr[f][g]/r_m_n; // Se divide cada valor en la posición (f,g) de la matriz por la raiz cuadrada del área de la imagen
						Qi[f][g]=Qi[f][g]/r_m_n; // Se divide cada valor en la posición (f,g) de la matriz por la raiz cuadrada del área de la imagen
						Qm[f][g]=sqrt(Qr[f][g]*Qr[f][g]+Qi[f][g]*Qi[f][g]); // Llenado de la matriz módulo, parece ser la magnitud de los números o "vectores" imaginarios a + bi.
						// Es la raiz cuadrada del valor en (f,g) al cuadrado de la matriz real + el cuadrado del valor en (f,g) de la matriz imaginaria.
			
					}
			
				}

				tc= clock() - tc; // Se toma el tiempo y se le resta el que tenía.
				TiempoC= ((double)tc / ((double)(CLOCKS_PER_SEC)))*1000; // Se pasa Tiempo C++ a milisegundos
				//cout<<endl<<"Tiempo C:"<<TiempoC;
				SetTextColor(hdc,RGB(212,85,0)); // Color para la letra
				SetBkColor(hdc,RGB(95,188,211));		// Color para el fondo del label de la letra
				TextOutA(hdc,M+170,(N/2),"Transformada en C++: ",20); // Texto para mostrar la transformada de Fourier en c++
				for (f=0;f<M;f++) // Primer ciclo para graficado de las imágenes de la transformada por medio de las matrices anteriormente calculadas
				{
					for (g=0;g<N;g++) // Segundo ciclo para graficado de las imágenes de la transformada por medio de las matrices anteriormente calculadas
					{
						SetPixel(hdc,f+M+330,g+20,RGB((int)Qm[f][g],(int)Qm[f][g],(int)Qm[f][g]));
						SetPixel(hdc,f+2*M+350,g+20,RGB((int)(fabs(Qr[f][g])),(int)(fabs(Qr[f][g])),(int)(fabs(Qr[f][g]))));
						SetPixel(hdc,f+3*M+370,g+20,RGB((int)(fabs(Qi[f][g])),(int)(fabs(Qi[f][g])),(int)(fabs(Qi[f][g]))));
					}
				}
			
		

				ta= clock(); // Se incia a tomar el tiempo que demora en hacer el cálculo.
			//Aquí irá cálculo en ASM... RECUERDA INICIALIZAR LAS MATRICES EN 0
				__asm{

				finit			; Inicializamos la pila

	
				mov f, 0			; Variable para el manejo de la posición en filas. Número de fila
					PrimerWhile:	; Primer ciclo, manejo y avance en Filas
					mov eax,f		; Mueve número de fila actual
					cmp eax,M		; Compara máximo de filas con el número de fila actual
					jge FinWhile1	; Salta a etiqueta fin del primer while si el número de fila actual es mayor que el máximo de filas
	
					mov ebx, TYPE Qra	; Se mueve el valor en bytes del tamaño del tipo de dato, o declaración, de la matriz a ebx
					imul ebx, f		; Se multiplica este valor por el número de fila (Para movimiento por filas completas)
					;mov ecx, ebx	; Se mete en ecx la cuenta de las columnas
					mov columnasfg,ebx	; Se mueve el valor de la cuenta a columnasfg
	
					mov g, 0			; Variable para el manejo de la posición en columnas. Número de columna.
						SegundoWhile:	; Inicio segundo ciclo para avance de posiciones (Columnas)
						mov eax, g		; Mueve número de columna actual
						cmp eax, N		; Compara máximo de columnas con el número de columna actual
						jge FinWhile2	; Salta a etiqueta fin del segundo while si el número de columna actual es mayor que el máximo de columnas
		
						mov ebx, 8	; Se mueve el valor de 8 a ebx, el 8 es por el tamaño en bytes de la declaración QWORD que se utilizará más adelante
						imul ebx, g	; Se multiplica por g el valor en ebx
						mov edx, ebx	; se mueve a edx la fila especificada
		
						mov ebx, columnasfg		; Movemos a ebx el valor de columnasfg. ebx = (TYPE Qra)*f
						add ebx, edx			; Movemos a ebx la suma respectiva para la posición (f,g) de la matriz
						mov FilaColumnaQr,ebx	; se mueve lo de ebx a FilaColumnafg para manejar bien la posición en esa variable
						mov esi, FilaColumnaQr	; Movemos a esi lo que sería el desplazamiento total en el arreglo, para así manejar este registro
											; como índice en la matriz
				
						fldz					; Metemos a la pila el valor cero... st(0)=0
						fstp  Qra[qword ptr esi]	; Se saca de la pila a st(0) y se guarda en la posición de memoria [qword ptr Qra + FilaColumnaQr],
											;la etiqueta qword ptr se usa para referenciar el tipo de memoria (clasificado) que se va a acceder
				
						mov ebx, 8			; Se mueve el valor de 8 a ebx, el 8 es por el tamaño en bytes de la declaración QWORD que se utilizará más adelante
						imul ebx, g			; Se multiplica por el número de columna
						mov edx, ebx			; se mueve a edx la fila especificada
		
						mov ebx, columnasfg		; Movemos el valor de Columnasfg. ebx = (TYPE Qra)*f
						add ebx, edx			; Movemos a ebx la suma respectiva para la posición (f,g) de la matriz
						mov FilaColumnaQi,ebx	; se mueve lo de ebx a FilaColumnaQi para manejar bien la posición en esa variable
						mov esi, FilaColumnaQi	; Se mueve al registro esi el cálculo hecho que nos da como resultado el índice para el desplazamiento total en los arreglos manejados
						fldz					; Se inserta cero en la pila... st(0)=0
						fstp Qia[qword ptr esi]	; Se le da a la matriz Qia (imaginaria) el valor de cero en todas sus posiciones.
		
		
						mov x, 0			; Nueva variable para manejo de posiciones, esta viene siendo el número de fila para las próximas matrices a manejar.
							TercerWhile:	; Tercer ciclo, manejo y avance en Filas
							mov eax, x	; Mueve número de fila actual.	
							cmp eax,M		; Compara máximo de filas con el número de fila actual
							jge FinWhile3	; Salta a etiqueta fin del Tercer while si el número de fila actual es mayor que el máximo de filas

							mov ebx, TYPE dot	; Se mueve al registro ebx, el tipo de dato de la declaración del arreglo. 
							imul ebx, x		; Se multiplica por el número de fila actual
							mov ecx, ebx		; Se mete en ecx la cuenta de las columnas
							mov columnasxy,ecx	; Se mueve la cuenta a columnasxy
					
							mov y, 0			; Nueva variable para manejo de posiciones, esta viene siendo el número de columna para las próximas matrices a manejar.
								CuartoWhile:	; Inicio segundo ciclo para avance de posiciones (Columnas)
								mov eax, y	; Mueve número de columna actual
								cmp eax, N	; Compara máximo de columnas con el número de columna actual
								jge FinWhile4	; Salta a etiqueta fin del segundo while si el número de columna actual es mayor que el máximo de columnas

	
									FILD f	; Se añade la f actual a la pila con el comando para variables enteras... st(0)=f
									FILD x	; Se añade la x actual a la pila con el comando para variables enteras... st(0)=x, st(1)=f
									FMUL		; Se multiplica st(1) * st(0) se almacena en st(1) y se desapila st(0)... st(0)= f*x

									FILD M	; Se añade el máximo de filas a la pila con el comando para variables enteras... st(0)=M, st(0)= f*x 
									FDIV		; Se divide st(1) entre st(0) se almacena en st(1) y se desapila st(0)... st(0)=f*x/M
									FILD g	; Se añade la f actual a la pila con el comando para variables enteras... st(0)=g, st(1)=f*x/M
									FILD y	; Se añade la f actual a la pila con el comando para variables enteras... st(0)=y, st(1)=g, st(2)=f*x/M
									FMUL		; Se multiplica st(1) * st(0) se almacena en st(1) y se desapila st(0)... st(0)= g*y, st(1)=f*x/M
									FILD N	; Se añade el máximo de columnas a la pila con el comando para variables enteras... st(0)=N
									FDIV		; Se divide st(1) entre st(0) se almacena en st(1) y se desapila st(0)... st(0)=y*g/N, st(1)=f*x/M
									FADD		; Se suma st(1) con st(0), se almacena en st(1) y se desapila st(0)... st(0)=(y*g/N) + (f*x/M)
									FLDPI	; Se añade pi a la pila... st(0)=pi, st(1)=(y*g/N) + (f*x/M)
									FADD st(0), st(0)	; Se suma st(0) con el mismo, 2 veces st(0)... st(0)=2pi, st(1)=(y*g/N) + (f*x/M)
									FMUL		; Se multiplica st(1) * st(0) se almacena en st(1) y se desapila st(0)... st(0)=2pi*[(y*g/N) + (f*x/M)]
									FSTP ang	; Se inserta el resultado guardado en st(0) en la variable ang y se desapila st(0)... Pila vacía
	
									mov ebx, 8	; Se mueve el valor de 8 a ebx, el 8 es por el tamaño en bytes de la declaración QWORD que se utilizará más adelante
									imul ebx, y	; Se multiplica por el valor en y a ebx
									mov edx, ebx	; se mueve a edx la fila especificada
					
									mov ebx, columnasxy		; movemos el valor anteriormente hallado de columnasxy
									add ebx, edx			; Movemos a ebx la suma respectiva para la posición (f,g) de la matriz
									mov FilaColumnaDt,ebx	; se mueve lo de ebx a FilaColumnaQi para manejar bien la posición en esa variable
									mov esi, FilaColumnaDt	; Movemos la traslación total en el arreglo al registro esi para manejo de índices

									fld dot[qword ptr esi]	; Se añade Dot[FilaColumnaDt] a la pila... st(0)=Dot[FilaColumnaDt]
									fld ang				; Se añade ang a la pila... st(0)=ang, st(1)=Dot[FilaColumnaDt]
									FCOS					; Se le saca el coseno a st(0) y se almacena ahí mismo... st(0)=cos(ang), st(1)=Dot[FilaColumnaDt]
									FMUL					; Se multiplica st(1) * st(0) se almacena en st(1) y se desapila st(0)... st(0)=Dot[FilaColumnaDt]*cos(ang)
					
									mov esi, FilaColumnaQr	; Movemos a esi el valor la traslación total en la matriz Qr

									fld Qra[qword ptr esi]	; Se añade Qra[FilaColumnaQr] a la pila... st(0)=Qra[FilaColumnaQr]... st(0)=Qra[FilaColumnaQr], st(1)=Dot[FilaColumnaDt]*cos(ang)
									FADD					; Se suma st(1) con st(0), se almacena en st(1) y se desapila st(0)... st(0)=Qra[FilaColumnaQr] + Dot[FilaColumnaDt]*cos(ang)
									fstp Qra[qword ptr esi]	; Se almacena el valor en st(0) en Qra[FilaColumnaQr] y se desapila st(0)... Pila vacía
	
									mov esi, FilaColumnaQi	; Movemos a esi el valor la traslación total en la matriz Qi

									fld Qia[qword ptr esi]	; Se añade Qra[FilaColumnaQr] a la pila... st(0)=Qra[FilaColumnaQr]... st(0)=Qra[FilaColumnaQi]
					
									mov esi, FilaColumnaDt	; Movemos la traslación total en el arreglo al registro esi para manejo de índices

									fld dot[qword ptr esi]	; Se añade Dot[FilaColumnaDt] a la pila... st(0)=Dot[FilaColumnaDt], st(1)=Qra[FilaColumnaQi]
									fld ang				; Se añade ang a la pila... st(0)=ang, st(1)=Dot[FilaColumnaDt], st(2)=Qra[FilaColumnaQi]
									FSIN					; Se le saca el seno al st(0) y se almacena ahí mismo st(0)=sen(ang), st(1)=Dot[FilaColumnaDt], st(2)=Qra[FilaColumnaQi]
									FMUL					; Se multiplica st(1) * st(0) se almacena en st(1) y se desapila st(0)... st(0)=Dot[FilaColumnaDt]*sen(ang), st(1)=Qra[FilaColumnaQi]
					
									FSUB					; Se hace st(1) - st(0) se almacena en st(1) y se desapila st(0)... st(0)= Qra[FilaColumnaQi] - Dot[FilaColumnaDt]*sen(ang)

									mov esi, FilaColumnaQi	; Movemos a esi el valor la traslación total en la matriz Qi
									fstp Qia[qword ptr esi]	; Se almacena el valor en st(0) en Qia[FilaColumnaQi] y se desapila st(0)... Pila vacía

								inc y				; Se incrementa en 1 a y
								jmp CuartoWhile		; Salta a la etiqueta CuartoWhile para empezar de  nuevo el cuarto ciclo while
								FinWhile4:			; Etiqueta para el final del cuarto while
		
		
							inc x					; Se incrementa en 1 a x
							jmp TercerWhile			; Salta a la etiqueta TercerWhile para empezar de nuevo el Tercer ciclo while
							FinWhile3:				; Etiqueta para el final del Tercer while
			
			
						mov esi, FilaColumnaQr			; Movemos a esi el valor la traslación total en la matriz Qr
						fld Qra[qword ptr esi]			; Se añade Qra[FilaColumnaQr] a la pila... st(0)=Qra[FilaColumnaQr]
						fld r_m_n						; Se añade la variable r_m_n a la pila... st(0)= r_m_n, st(1)=Qra[FilaColumnaQr]
						FDIV							; Se divide st(1) entre st(0) se almacena en st(1) y se desapila st(0)... st(0)=Qra[FilaColumnaQr]/r_m_n
						fstp Qra[qword ptr esi]			; Se almacena en Qra[FilaColumnaQr] el valor de st(0) y se desapila... Pila vacía.
		
						mov esi, FilaColumnaQi			; Movemos a esi el valor la traslación total en la matriz Qi

						fld Qia[qword ptr esi]			; Se añade Qia[FilaColumnaQi] a la pila... st(0)=Qia[FilaColumnaQi]
						fld r_m_n						; Se añade la variable r_m_n a la pila... st(0)= r_m_n, st(1)=Qia[FilaColumnaQi]
						FDIV							; Se divide st(1) entre st(0) se almacena en st(1) y se desapila st(0)... st(0)=Qia[FilaColumnaQi]/r_m_n
						fstp Qia[qword ptr esi]			; Se almacena en Qra[FilaColumnaQr] el valor de st(0) y se desapila... Pila vacía.

						mov esi, FilaColumnaQr			; Movemos a esi el valor la traslación total en la matriz Qr

						fld Qra[qword ptr esi]			; Se añade Qra[FilaColumnaQr] a la pila... st(0)=Qra[FilaColumnaQr] 
						FMUL st(0), st(0)				; Se eleva st(0) al cuadrado... st(0)=(Qra[FilaColumnaQr])^2
		
						mov esi, FilaColumnaQi			; Movemos a esi el valor la traslación total en la matriz Qi

						fld Qia[qword ptr esi]			; Se añade Qia[FilaColumnaQi] a la pila... st(0)=Qia[FilaColumnaQi] 
						FMUL st(0), st(0)				; Se eleva st(0) al cuadrado... st(0)=(Qia[FilaColumnaQi])^2
		
						FADD							; Se suma st(1) + st(0) se almacena en st(1) y se desapila en st(0)... st(0)=(Qra[FilaColumnaQr])^2 + (Qia[FilaColumnaQi])^2
						FSQRT						; Se le saca raiz cuadrada a st(0) y se almacena ahí mismo... st(0) = [(Qra[FilaColumnaQr])^2 + (Qia[FilaColumnaQi])^2]^(1/2)
		
						mov ebx, 8					; Se mueve el valor de 8 a ebx, el 8 es por el tamaño en bytes de la declaración QWORD que se utilizará más adelante
						imul ebx, g					; Se multiplica por g el valor en ebx
						mov edx, ebx					; se mueve a edx la fila especificada
		
						mov ebx, columnasfg				; Se mueve a ebx el valor en columnasfg
						add ebx, edx					; Movemos a ebx la suma respectiva para la posición (f,g) de la matriz
						mov FilaColumnaQm,ebx			; se mueve lo de ebx a FilaColumnaQi para manejar bien la posición en esa variable
						mov esi, FilaColumnaQm			; Movemos al esi el valor en FilaColumnaQm para el manejo de los índices

						fstp Qma[qword ptr esi]			; Se almacena en Qma[FilaColumnaQm] el valor en st(0) y se desapila... Pila vacía
				
						inc g				; Se incrementa g en 1	
						jmp SegundoWhile		; Salta a la etiqueta SegundoWhile para reiniciar el segundo ciclo while
						FinWhile2:			; Etiqueta para el final del Segundo while
		
					inc f					; Se incrementa f en 1
					jmp PrimerWhile			; Salta a la etiqueta SegundoWhile para reiniciar el Primer ciclo while
					FinWhile1:				; Etiqueta para el final del Primer while
			
				}
				ta= clock() - ta; // Se hace la resta del tiempo en ese preciso momento y el que se tenía de referencia.
				TiempoAssembly= ((double)ta / ((double)CLOCKS_PER_SEC)) * 1000; // Se pasa TiempoASM a milisegundos

				SetTextColor(hdc,RGB(0,128,128)); // Color para la letra
				SetBkColor(hdc,RGB(95,188,211));		// Color para el fondo del label de la letra
				TextOutA(hdc,M+170,(N/2)+N+20,"Transformada en ASM: ",20); // Texto para mostrar la transformada de Fourier en asm
			//Como ya terminó el Cálculo lo mostramos el resultado en assembly.
		
				for (f=0;f<M;f++)
				{
					for (g=0;g<N;g++)
					{
						SetPixel(hdc,f+M+330,g+N+40,RGB((int)Qma[f][g],(int)Qma[f][g],(int)Qma[f][g]));
						SetPixel(hdc,f+2*M+350,g+N+40,RGB((int)(fabs(Qra[f][g])),(int)(fabs(Qra[f][g])),(int)(fabs(Qra[f][g]))));
						SetPixel(hdc,f+3*M+370,g+N+40,RGB((int)(fabs(Qia[f][g])),(int)(fabs(Qia[f][g])),(int)(fabs(Qia[f][g]))));
					}
				}

				//cout<<endl<<"tiempo ASM: "<<TiempoAssembly;

			
				// Se llena el buffer para mostrar por un messageBox
				TIEMPOS<<"Tiempo C++: "<<TiempoC<<" milisegundos"<<endl
					<<"Tiempo ASM: "<<TiempoAssembly<<" milisegundos"<<endl
					<<"Diferencia: "<<abs(TiempoC-TiempoAssembly)<<" milisegundos"; 

				MessageBox(NULL,TIEMPOS.str().c_str(),_T("Tiempos"),NULL);

				
*/

EndPaint(hWnd, &ps);
			}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AQUI TERMINAN LOS CALCULOS Y TODO EL PROCESO
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		}else if(Salir == (HWND)lParam){
			DestroyWindow(hWnd);
		}else if(Instruc == (HWND)lParam){
			// Se crea una variable llamada mensaje con lo que irá dentro de el botón Instrucciones.
			TCHAR Mensaje[]=_T("BIENVENIDO\n\nEste programa está diseñado para calcular la transformada discreta de Fourier\nen imágenes de mapas de bits (Bitmap-BMP).\n\nEl programa consta de varias operaciones, que están comandadas por los botones\nen la pantalla principal, estos son:\n\nExaminar: Abre la pantalla de búsqueda para cargar el archivo que se desee,\n	una vez escogido es montado en la pantalla principal y comienza\n	a hacer los cálculos, a mostrar las transformadas y los tiempos \n	de las mismas.\n\nAcerca de: Muestra una ventana de mensaje con los créditos del programa.\n\nSalir: Termina el programa y despliega un mensaje de despedida.");
			MessageBox(NULL,Mensaje,_T("Instrucciones"),NULL);
		}else if(AcercaDe == (HWND)lParam){
			TCHAR Mensaje2[]=_T("Transformada Bidimensional Discreta de Fourier\n	Fecha de Creación Junio/10/2013\n	Versión 1.0.2.32\n	Arquitectura de computadores\n	Semestre 01-2013\n	Desarrolladores:\n		Cristian Camilo Morales Jiménez\n		Steven Velasquez Chancí\n\n	© 2013 Arquitecture");
			MessageBox(NULL,Mensaje2,_T("Acerca De..."),NULL);
		}
	    break;
    case WM_PAINT:
		hdc=BeginPaint (hWnd,&ps);
		EndPaint(hWnd, &ps);
	   break;
    
    case WM_DESTROY:
                MessageBox(NULL,
            _T("FIN DEL PROGRAMA "),
            _T("Mensaje"),
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
