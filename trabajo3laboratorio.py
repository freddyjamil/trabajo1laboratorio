import subprocess
import pandas as pd
import matplotlib.pyplot as plt
try:
    from IPython.display import display
except ImportError:
    display = print

codigo_cpp = r'''
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <bitset>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <time.h>
using namespace std;

#define rand_end 1000
#define n_pob 10        // tamaño de la población (par)
#define n_iter 100      // generaciones
#define len 10          // bits por individuo
#define l unsigned long

l to_int(string n) { return bitset<len>(n).to_ulong(); }
string to_binario(l x) { return bitset<len>(x).to_string(); }
l funcion(int x) { return (l) pow(x, 2); }

void mutacion2(string &individuo1, string &individuo2) {
	string seed = "0001000000";
	for (size_t i = 0; i < individuo1.length(); ++i) {
		int r = rand() % seed.length();
		if (seed[r] == '1') swap(individuo1[i], individuo2[i]);
	}
}

vector<string> cruzamiento2(vector<string> &padres) {
	vector<string> nueva_generacion;
	for (int i = 0; i < n_pob / 2; ++i) {
		string a1 = padres[rand() % padres.size()];
		string a2 = padres[rand() % padres.size()];
		mutacion2(a1, a2);
		nueva_generacion.push_back(a1);
		nueva_generacion.push_back(a2);
	}
	return nueva_generacion;
}

void seleccion2(vector<string> &pob, vector<float> &fun, float media) {
	vector<string> padres;
	int i = 0, j = 0;
	while (i < n_pob && j != n_pob) {
		const int rep = media > 0 ? (int) (fun[j] / media + 0.5) : 1;
		for (int cont = 0; cont < rep; ++cont) padres.push_back(pob[j]);
		i += rep;
		j++;
	}
	if (padres.empty()) padres = pob;
	pob = cruzamiento2(padres);
}

void grafica_ascii(const vector<float> &mejor, const vector<float> &prom) {
	float maximo = *max_element(mejor.begin(), mejor.end());
	if (maximo <= 0) maximo = 1;
	const int ancho = 40;
	int paso = max(1, (int) mejor.size() / 10);
	cout << "\nGRAFICA COMPARATIVA   (# = mejor individuo, + = promedio)\n";
	for (size_t g = 0; g < mejor.size(); g += paso) {
		int b = (int) (mejor[g] / maximo * ancho);
		int p = (int) (prom[g] / maximo * ancho);
		cout << "Gen " << setw(3) << g + 1 << " |" << string(b, '#') << " " << (l) mejor[g] << "\n"
		     << "        |" << string(p, '+') << " " << (l) prom[g] << "\n";
	}
}

int main() {
	srand(time(NULL));
	vector<string> poblacion;
	vector<float> hist_mejor, hist_prom;
	string mejor_global = "";
	l mejor_global_fx = 0;

	for (int i = 0; i < n_pob; ++i) poblacion.push_back(to_binario(rand() % rand_end));

	for (int it = 0; it < n_iter; ++it) {
		vector<float> fun_aptitud;
		float media = 0, mejor_gen = 0;
		for (int j = 0; j < n_pob; ++j) {
			l fx = funcion(to_int(poblacion[j]));
			fun_aptitud.push_back((float) fx);
			media += fx;
			if (fx > mejor_gen) mejor_gen = (float) fx;
			if (fx > mejor_global_fx) { mejor_global_fx = fx; mejor_global = poblacion[j]; }
		}
		media /= n_pob;
		hist_mejor.push_back(mejor_gen);
		hist_prom.push_back(media);

		seleccion2(poblacion, fun_aptitud, media);
		poblacion[0] = mejor_global;   // elitismo (borra esta linea si no lo quieres)
	}

	l x = to_int(mejor_global);
	cout << "==================== RESULTADO FINAL ====================\n"
	     << "Generaciones ejecutadas  : " << n_iter << "\n"
	     << "Tamano de poblacion      : " << n_pob << "\n"
	     << "Mejor individuo (binario): " << mejor_global << "\n"
	     << "Mejor individuo (x)      : " << x << "\n"
	     << "f(x) = x^2               : " << mejor_global_fx << "\n"
	     << "Maximo teorico (x=" << (1 << len) - 1 << ") : " << funcion((1 << len) - 1) << "\n";
	grafica_ascii(hist_mejor, hist_prom);

	ofstream csv("resultados.csv");
	csv << "generacion,mejor,promedio\n";
	for (size_t g = 0; g < hist_mejor.size(); ++g)
		csv << g + 1 << "," << hist_mejor[g] << "," << hist_prom[g] << "\n";
}
'''

# 1) Guardar, compilar y ejecutar el C++
with open('programa.cpp', 'w') as f:
    f.write(codigo_cpp)
comp = subprocess.run(['g++', '-O2', '-std=c++17', 'programa.cpp', '-o', 'programa'],
                      capture_output=True, text=True)
if comp.returncode != 0:
    print("ERROR DE COMPILACION:\n", comp.stderr)
else:
    print(subprocess.run(['./programa'], capture_output=True, text=True).stdout)

    # 2) Leer los datos y armar la tabla
    d = pd.read_csv('resultados.csv')
    paso = max(1, len(d) // 10)
    tabla = d.iloc[::paso]
    if tabla.generacion.iloc[-1] != d.generacion.iloc[-1]:
        tabla = pd.concat([tabla, d.iloc[[-1]]])
    tabla = tabla.reset_index(drop=True)
    tabla.columns = ['Generación', 'Mejor f(x)', 'Promedio f(x)']
    tabla = tabla.astype(int)

    print("TABLA DE RESULTADOS")
    display(tabla)

    # 3) Gráfica + tabla en una sola imagen
    filas = [[r[0], f"{r[1]:,}", f"{r[2]:,}"] for r in tabla.itertuples(index=False)]
    fig, (ax, ax_t) = plt.subplots(2, 1, figsize=(8, 9), gridspec_kw={'height_ratios': [2, 3]})
    ax.plot(d.generacion, d.mejor, label='Mejor individuo')
    ax.plot(d.generacion, d.promedio, label='Promedio')
    ax.set_xlabel('Generación')
    ax.set_ylabel('f(x) = x²')
    ax.set_title('Mejor vs. promedio por generación')
    ax.legend()
    ax.grid(alpha=0.3)

    ax_t.axis('off')
    t = ax_t.table(cellText=filas, colLabels=list(tabla.columns), loc='center', cellLoc='center')
    t.auto_set_font_size(False)
    t.set_fontsize(10)
    t.scale(1, 1.5)
    for j in range(3):
        t[0, j].set_facecolor('#4472C4')
        t[0, j].set_text_props(color='white', weight='bold')
    for i in range(2, len(filas) + 1, 2):
        for j in range(3):
            t[i, j].set_facecolor('#E9EFF7')

    fig.suptitle(f"Mejor resultado global: f(x) = {int(d.mejor.max()):,}", fontsize=12)
    plt.tight_layout()
    plt.savefig('grafica_tabla.png', dpi=150)
    plt.show()
