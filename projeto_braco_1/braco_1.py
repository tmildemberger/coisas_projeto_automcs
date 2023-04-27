import cadquery as cq
from cadquery import exporters
import teardrop  # Adds the teardrop function to cadquery.Workplane

# em milímetros
comprimento_braco = 120
largura_braco = 16
altura_braco = 8
profundidade_acoplamento = 4.5

raio_base_acoplamento = 12.5
raio_base_motor = 15


raio_roda = 12
largura_roda = 6
eixo_roda = 7
raio_eixo_roda = 1
raio_furo_eixo = raio_eixo_roda + 0.1

altura_motor = 40


dist_suporte_roda = 20
raio_menor_suporte_roda = 6
altura_suporte_roda = altura_motor - raio_roda + altura_braco
largura_suporte_roda = 8

raio_arrendondamentos = 1.2

w = cq.Workplane('XY').box(comprimento_braco,
                           largura_braco,
                           altura_braco,
                           (False, True, False))

#w = w.edges('|X').fillet(1)
wc = cq.Workplane('XY').moveTo(comprimento_braco, 0).circle(raio_base_motor).extrude(altura_braco, combine=False)
wc = wc.edges().fillet(raio_arrendondamentos)
w = w.edges('|X').fillet(raio_arrendondamentos)
w = w.union(wc).clean()

altura_ate_furo_roda = altura_braco - profundidade_acoplamento + altura_motor - raio_roda / 2
w_roda = cq.Workplane('YZ', (comprimento_braco - dist_suporte_roda + largura_suporte_roda / 2 + largura_roda / 2 + eixo_roda / 7, 0, altura_ate_furo_roda)).circle(raio_roda).extrude(largura_roda / 2, both=True).edges().fillet(1.0).circle(raio_eixo_roda).extrude(-eixo_roda - largura_roda/2)
#w = w.moveTo(comprimento_braco - dist_suporte_roda, 0).box(largura_suporte_roda, largura_braco, altura_suporte_roda, (True, True, False))
h_max = altura_ate_furo_roda + raio_menor_suporte_roda
pts = [
    (-largura_braco / 2, 0),
    (largura_braco / 2, 0),
    (largura_braco / 2, h_max - largura_braco / 2),
    (0, h_max),
    (-largura_braco / 2, h_max - largura_braco / 2),
    (-largura_braco / 2, 0)
]
w_sup = cq.Workplane('YZ', (comprimento_braco - dist_suporte_roda, 0, 0)).polyline(pts).close().extrude(largura_suporte_roda / 2, both=True).edges('(not >Z) and |X').fillet(raio_arrendondamentos).edges('>Z').fillet(raio_menor_suporte_roda).faces('|X').edges().fillet(raio_arrendondamentos)
w_sup = w_sup.moveTo(0, altura_ate_furo_roda).teardrop(raio_furo_eixo).cutThruAll()
#w = w.edges('|X').fillet(raio_arrendondamentos)
#w = w.edges('|Z').fillet(raio_arrendondamentos)
#w = w.edges('|Y').fillet(raio_arrendondamentos)
w = w.union(w_sup).clean()
w = w.edges('|Y').fillet(raio_arrendondamentos)

from testes_acoplamento import acoplamento
obj_acoplamento = acoplamento(profundidade_acoplamento, altura_braco, raio_base_acoplamento, raio_arrendondamentos)
neg = cq.Workplane('XY').circle(obj_acoplamento[1] - raio_arrendondamentos).extrude(altura_braco)
w = w.moveTo(0, 0).cut(neg).union(obj_acoplamento[0])


show_object(w)
show_object(w_roda)

if True:
    exporters.export(w, 'braco_1.stl')
