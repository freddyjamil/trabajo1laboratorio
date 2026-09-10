#include <GL/freeglut.h>
#include <vector>
#include <queue>
#include <stack>
#include <cmath>
#include <random>
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <limits>
#include <cstdlib>

using namespace std;
using Clock = chrono::high_resolution_clock;


const int N = 50;
const double PROB_ELIMINAR = 0.20;
const int CELL = 12;

struct Arista { int hacia; double peso; };

vector<vector<Arista>> grafo(N * N);

int inicioNodo = 0;
int finNodo    = N * N - 1;
vector<bool> visitado(N * N, false);
vector<int>  camino;
double ultimoCosto = -1;
long long ultimosVisitados = 0;
double ultimoTiempoMs = 0;
string ultimoAlgoritmo = "Ninguno";

inline int idx(int f, int c) { return f * N + c; }
inline int filaDe(int nodo)  { return nodo / N; }
inline int colDe(int nodo)   { return nodo % N; }

double distanciaEuclidiana(int a, int b) {
    int fa = filaDe(a), ca = colDe(a);
    int fb = filaDe(b), cb = colDe(b);
    double df = fa - fb, dc = ca - cb;
    return sqrt(df * df + dc * dc);
}

void construirGrafo() {
    for (auto &lista : grafo) lista.clear();

    mt19937 rng(random_device{}());
    uniform_real_distribution<double> prob(0.0, 1.0);

    int offF[4] = {0, 1, 1, 1};
    int offC[4] = {1, -1, 0, 1};

    for (int f = 0; f < N; f++) {
        for (int c = 0; c < N; c++) {
            int u = idx(f, c);
            for (int d = 0; d < 4; d++) {
                int nf = f + offF[d];
                int nc = c + offC[d];
                if (nf < 0 || nf >= N || nc < 0 || nc >= N) continue;
                if (prob(rng) < PROB_ELIMINAR) continue;

                int v = idx(nf, nc);
                bool esDiagonal = (offF[d] != 0 && offC[d] != 0);
                double peso = esDiagonal ? sqrt(2.0) : 1.0;

                grafo[u].push_back({v, peso});
                grafo[v].push_back({u, peso});
            }
        }
    }
    cout << "[Grafo regenerado] " << N << "x" << N << " nodos, "
         << (int)(PROB_ELIMINAR * 100) << "% de aristas eliminadas.\n";
}

vector<int> reconstruirCamino(const vector<int> &padre, int fin) {
    vector<int> c;
    if (padre[fin] == -2) return c;
    int actual = fin;
    while (actual != -1) {
        c.push_back(actual);
        actual = padre[actual];
    }
    reverse(c.begin(), c.end());
    return c;
}

void busquedaBFS() {
    auto t0 = Clock::now();
    vector<int> padre(N * N, -2);
    vector<bool> visto(N * N, false);
    queue<int> q;

    q.push(inicioNodo);
    visto[inicioNodo] = true;
    padre[inicioNodo] = -1;

    while (!q.empty()) {
        int u = q.front(); q.pop();
        if (u == finNodo) break;
        for (auto &a : grafo[u]) {
            if (!visto[a.hacia]) {
                visto[a.hacia] = true;
                padre[a.hacia] = u;
                q.push(a.hacia);
            }
        }
    }

    vector<int> c = reconstruirCamino(padre, finNodo);
    double costo = 0;
    for (size_t i = 1; i < c.size(); i++) costo += distanciaEuclidiana(c[i-1], c[i]);

    auto t1 = Clock::now();
    visitado = visto;
    camino = c;
    ultimoCosto = c.empty() ? -1 : costo;
    ultimosVisitados = count(visto.begin(), visto.end(), true);
    ultimoTiempoMs = chrono::duration<double, milli>(t1 - t0).count();
    ultimoAlgoritmo = "BFS (ciega)";
}

void busquedaDFS() {
    auto t0 = Clock::now();
    vector<int> padre(N * N, -2);
    vector<bool> visto(N * N, false);
    stack<int> pila;

    pila.push(inicioNodo);
    padre[inicioNodo] = -1;

    while (!pila.empty()) {
        int u = pila.top(); pila.pop();
        if (visto[u]) continue;
        visto[u] = true;
        if (u == finNodo) break;

        for (auto &a : grafo[u]) {
            if (!visto[a.hacia]) {
                if (padre[a.hacia] == -2) padre[a.hacia] = u;
                pila.push(a.hacia);
            }
        }
    }

    vector<int> c = reconstruirCamino(padre, finNodo);
    double costo = 0;
    for (size_t i = 1; i < c.size(); i++) costo += distanciaEuclidiana(c[i-1], c[i]);

    auto t1 = Clock::now();
    visitado = visto;
    camino = c;
    ultimoCosto = c.empty() ? -1 : costo;
    ultimosVisitados = count(visto.begin(), visto.end(), true);
    ultimoTiempoMs = chrono::duration<double, milli>(t1 - t0).count();
    ultimoAlgoritmo = "DFS (ciega)";
}

void busquedaHillClimbing() {
    auto t0 = Clock::now();
    vector<bool> visto(N * N, false);
    vector<int> c;

    int actual = inicioNodo;
    visto[actual] = true;
    c.push_back(actual);
    bool exito = (actual == finNodo);

    while (actual != finNodo) {
        int mejorVecino = -1;
        double mejorH = numeric_limits<double>::infinity();

        for (auto &a : grafo[actual]) {
            if (visto[a.hacia]) continue;
            double h = distanciaEuclidiana(a.hacia, finNodo);
            if (h < mejorH) {
                mejorH = h;
                mejorVecino = a.hacia;
            }
        }

        if (mejorVecino == -1) break;

        visto[mejorVecino] = true;
        actual = mejorVecino;
        c.push_back(actual);

        if (actual == finNodo) { exito = true; break; }
    }

    double costo = 0;
    for (size_t i = 1; i < c.size(); i++) costo += distanciaEuclidiana(c[i-1], c[i]);

    auto t1 = Clock::now();
    visitado = visto;
    camino = exito ? c : vector<int>();
    ultimoCosto = exito ? costo : -1;
    ultimosVisitados = count(visto.begin(), visto.end(), true);
    ultimoTiempoMs = chrono::duration<double, milli>(t1 - t0).count();
    ultimoAlgoritmo = exito ? "Hill Climbing (heuristica)"
                             : "Hill Climbing (heuristica) - atrapado en optimo local, sin camino";
}

void busquedaAEstrella() {
    auto t0 = Clock::now();
    vector<double> g(N * N, numeric_limits<double>::infinity());
    vector<int> padre(N * N, -2);
    vector<bool> cerrado(N * N, false);

    using Par = pair<double, int>; // (f = g+h, nodo)
    priority_queue<Par, vector<Par>, greater<>> abierta;

    g[inicioNodo] = 0;
    padre[inicioNodo] = -1;
    abierta.push({distanciaEuclidiana(inicioNodo, finNodo), inicioNodo});

    while (!abierta.empty()) {
        auto [f, u] = abierta.top(); abierta.pop();
        if (cerrado[u]) continue;
        cerrado[u] = true;
        if (u == finNodo) break;

        for (auto &a : grafo[u]) {
            double nuevoG = g[u] + a.peso;
            if (nuevoG < g[a.hacia]) {
                g[a.hacia] = nuevoG;
                padre[a.hacia] = u;
                double h = distanciaEuclidiana(a.hacia, finNodo);
                abierta.push({nuevoG + h, a.hacia});
            }
        }
    }

    vector<int> c = reconstruirCamino(padre, finNodo);

    auto t1 = Clock::now();
    visitado = cerrado;
    camino = c;
    ultimoCosto = c.empty() ? -1 : g[finNodo];
    ultimosVisitados = count(cerrado.begin(), cerrado.end(), true);
    ultimoTiempoMs = chrono::duration<double, milli>(t1 - t0).count();
    ultimoAlgoritmo = "A* / A estrella (heuristica)";
}

void imprimirResultado() {
    cout << "----------------------------------------------------\n";
    cout << "Algoritmo:        " << ultimoAlgoritmo << "\n";
    cout << "Inicio -> Fin:    (" << filaDe(inicioNodo) << "," << colDe(inicioNodo)
         << ") -> (" << filaDe(finNodo) << "," << colDe(finNodo) << ")\n";
    if (ultimoCosto >= 0)
        cout << "Costo del camino: " << ultimoCosto << "   (" << camino.size() << " nodos)\n";
    else
        cout << "Costo del camino: NO SE ENCONTRO CAMINO\n";
    cout << "Nodos visitados:  " << ultimosVisitados << " de " << (N * N) << "\n";
    cout << "Tiempo:           " << ultimoTiempoMs << " ms\n";
    cout << "----------------------------------------------------\n";
}
void dibujarCelda(int f, int c, float r, float g, float b) {
    float x0 = c * CELL, y0 = f * CELL;
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x0, y0);
        glVertex2f(x0 + CELL - 1, y0);
        glVertex2f(x0 + CELL - 1, y0 + CELL - 1);
        glVertex2f(x0, y0 + CELL - 1);
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int f = 0; f < N; f++)
        for (int c = 0; c < N; c++)
            dibujarCelda(f, c, 0.93f, 0.93f, 0.93f);

    for (int n = 0; n < N * N; n++)
        if (visitado[n])
            dibujarCelda(filaDe(n), colDe(n), 0.55f, 0.72f, 1.0f);

    for (int n : camino)
        dibujarCelda(filaDe(n), colDe(n), 0.85f, 0.10f, 0.10f);

    dibujarCelda(filaDe(inicioNodo), colDe(inicioNodo), 0.0f, 0.75f, 0.0f);
    dibujarCelda(filaDe(finNodo), colDe(finNodo), 1.0f, 0.85f, 0.0f);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, N * CELL, N * CELL, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void teclado(unsigned char tecla, int x, int y) {
    switch (tecla) {
        case '1': busquedaBFS();          imprimirResultado(); glutPostRedisplay(); break;
        case '2': busquedaDFS();          imprimirResultado(); glutPostRedisplay(); break;
        case '3': busquedaHillClimbing(); imprimirResultado(); glutPostRedisplay(); break;
        case '4': busquedaAEstrella();    imprimirResultado(); glutPostRedisplay(); break;

        case 'g': case 'G':
            construirGrafo();
            fill(visitado.begin(), visitado.end(), false);
            camino.clear();
            ultimoAlgoritmo = "Ninguno (grafo recien generado)";
            ultimoCosto = -1;
            glutPostRedisplay();
            break;

        case 'c': case 'C':
            fill(visitado.begin(), visitado.end(), false);
            camino.clear();
            glutPostRedisplay();
            break;

        case 27:   // ESC
        case 'q': case 'Q':
            cout << "Cerrando el programa (solicitado por el usuario)...\n";
            exit(0);
            break;
    }
}

void mouse(int boton, int estado, int x, int y) {
    if (estado != GLUT_DOWN) return;
    int col = x / CELL;
    int fila = y / CELL;
    if (fila < 0 || fila >= N || col < 0 || col >= N) return;
    int nodo = idx(fila, col);

    if (boton == GLUT_LEFT_BUTTON) {
        inicioNodo = nodo;
        cout << "Nuevo nodo INICIO: (" << fila << "," << col << ")\n";
    } else if (boton == GLUT_RIGHT_BUTTON) {
        finNodo = nodo;
        cout << "Nuevo nodo FIN: (" << fila << "," << col << ")\n";
    }
    glutPostRedisplay();
}

void imprimirMenu() {
    cout << "======================================================\n";
    cout << " BUSQUEDA POR PROFUNDIDAD - Grafo no dirigido " << N << "x" << N << "\n";
    cout << "======================================================\n";
    cout << " Distancia recta = 1, diagonal = raiz(2) (Euclidiana)\n";
    cout << " Todas las busquedas se hacen sobre EL MISMO grafo.\n\n";
    cout << " Controles (con la ventana enfocada):\n";
    cout << "   1 -> BFS (ciega)              2 -> DFS (ciega)\n";
    cout << "   3 -> Hill Climbing (heuristica)  4 -> A* (heuristica)\n";
    cout << "   Clic izquierdo  -> fijar nodo INICIO\n";
    cout << "   Clic derecho    -> fijar nodo FIN\n";
    cout << "   G -> regenerar grafo   C -> limpiar colores\n";
    cout << "   ESC o Q -> salir\n";
    cout << "======================================================\n\n";
}

int main(int argc, char **argv) {
    imprimirMenu();
    construirGrafo();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(N * CELL, N * CELL);
    glutCreateWindow("Busqueda por profundidad - Grafo no dirigido (BFS/DFS/HillClimbing/A*)");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(teclado);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
