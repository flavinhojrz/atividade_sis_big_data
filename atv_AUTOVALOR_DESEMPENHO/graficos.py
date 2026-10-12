import csv
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

dados = {}
with open("resultados.csv") as arquivo:
    for linha in csv.reader(arquivo):
        if len(linha) != 5 or linha[0] == "n":
            continue
        n, p, steps = int(linha[0]), int(linha[1]), int(linha[2])
        dados[(n, p, steps)] = float(linha[3])

lista_n = sorted(set(k[0] for k in dados))
lista_p = sorted(set(k[1] for k in dados))
lista_steps = sorted(set(k[2] for k in dados))

texto = []
for n in lista_n:
    for steps in lista_steps:
        if (n, 1, steps) not in dados:
            continue
        t1 = dados[(n, 1, steps)]
        texto.append(f"n = {n} | steps = {steps}")
        texto.append(f"    {'p':<6}{'tempo (s)':<14}{'speedup':<12}{'eficiencia':<12}")
        for p in lista_p:
            if (n, p, steps) not in dados:
                continue
            tp = dados[(n, p, steps)]
            s = t1 / tp
            e = s / p
            texto.append(f"    {p:<6}{tp:<14.4f}{s:<12.2f}{e:<12.2f}")
        texto.append("")
tabelas = "\n".join(texto)

with open("tabelas.txt", "w") as arquivo:
    arquivo.write(tabelas)

try:
    with open("relatorio.txt") as arquivo:
        relatorio = arquivo.read()
    cabecalho = "6. TABELAS\n----------\n"
    inicio = relatorio.find(cabecalho)
    fim = relatorio.find("7. GRÁFICOS")
    if inicio != -1 and fim != -1:
        relatorio = relatorio[:inicio + len(cabecalho)] + "\n" + tabelas.rstrip() + "\n\n\n" + relatorio[fim:]
        with open("relatorio.txt", "w") as arquivo:
            arquivo.write(relatorio)
except FileNotFoundError:
    pass

cores = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
plt.rcParams.update({
    "font.size": 11,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.edgecolor": "#888888",
    "axes.labelcolor": "#333333",
    "xtick.color": "#555555",
    "ytick.color": "#555555",
    "axes.grid": True,
    "grid.color": "#e5e5e5",
    "grid.linewidth": 0.8,
    "legend.frameon": False,
})

def grafico(nome, titulo, eixo_y, series, rotulo, valor, ideal=None, log_y=False):
    fig, ax = plt.subplots(figsize=(8, 5))
    for i, item in enumerate(series):
        xs, ys = [], []
        for p in lista_p:
            v = valor(item, p)
            if v is not None:
                xs.append(p)
                ys.append(v)
        if not xs:
            continue
        ax.plot(xs, ys, color=cores[i % len(cores)], linewidth=2, marker="o", markersize=7,
                markeredgecolor="white", markeredgewidth=1.5, label=rotulo(item))
        ax.annotate(rotulo(item), (xs[-1], ys[-1]), xytext=(6, 0), textcoords="offset points",
                    va="center", fontsize=9, color="#333333")
    if ideal is not None:
        ax.plot(lista_p, [ideal(p) for p in lista_p], color="#999999", linewidth=1.5,
                linestyle="--", label="ideal")
    ax.set_xscale("log", base=2)
    ax.set_xticks(lista_p)
    ax.set_xticklabels([str(p) for p in lista_p])
    if log_y:
        ax.set_yscale("log")
        ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
        ax.yaxis.set_minor_formatter(FuncFormatter(lambda v, _: ""))
    ax.set_xlabel("número de processos (comm_sz)")
    ax.set_ylabel(eixo_y)
    ax.set_title(titulo, loc="left", fontsize=13, color="#111111")
    ax.legend(loc="best")
    fig.tight_layout()
    fig.savefig(nome, dpi=150)
    plt.close(fig)

def tempo(n, steps, p):
    return dados.get((n, p, steps))

def speedup(n, steps, p):
    if (n, 1, steps) in dados and (n, p, steps) in dados:
        return dados[(n, 1, steps)] / dados[(n, p, steps)]
    return None

def eficiencia(n, steps, p):
    s = speedup(n, steps, p)
    return None if s is None else s / p

steps_max = max(lista_steps)
n_max = max(lista_n)

grafico("grafico_tempo.png", f"Tempo de execução (steps = {steps_max})", "tempo (s)",
        lista_n, lambda n: f"n = {n}", lambda n, p: tempo(n, steps_max, p), log_y=True)
grafico("grafico_speedup.png", f"Speedup (steps = {steps_max})", "speedup",
        lista_n, lambda n: f"n = {n}", lambda n, p: speedup(n, steps_max, p), ideal=lambda p: p)
grafico("grafico_eficiencia.png", f"Eficiência (steps = {steps_max})", "eficiência",
        lista_n, lambda n: f"n = {n}", lambda n, p: eficiencia(n, steps_max, p), ideal=lambda p: 1)
grafico("grafico_speedup_steps.png", f"Speedup para diferentes steps (n = {n_max})", "speedup",
        lista_steps, lambda s: f"steps = {s}", lambda s, p: speedup(n_max, s, p), ideal=lambda p: p)

print(tabelas)
print("Graficos salvos: grafico_tempo.png, grafico_speedup.png, grafico_eficiencia.png, grafico_speedup_steps.png")
