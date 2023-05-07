import cadquery as cq
from cadquery import exporters
from cadquery.selectors import BoxSelector
from motor_2 import motor_2, motor_2_dim
from testes_acoplamento_motor_2 import acoplamento_motor_2

raio_arredondamentos = 1.6 / 2

profundidade_acoplamento = 7.2
profundidade_base_acoplamento = 2.8

# em milímetros
comprimento_braco = 130 - raio_arredondamentos
largura_braco = 12 - 2 * raio_arredondamentos
altura_braco = profundidade_acoplamento + profundidade_base_acoplamento - 2 * raio_arredondamentos

espacamento_base_acoplamento = 0.8

comprimento_prender_ima = 24
raio_ondulacao = 1

raio_base_acoplamento_2 = 10 - raio_arredondamentos


motor_2_dim = motor_2_dim()
w_motor = motor_2((motor_2_dim.raio - 6.5, 0, -motor_2_dim.altura -9.5 + profundidade_acoplamento))

show_object(w_motor)

acoplamento = acoplamento_motor_2(profundidade_acoplamento, profundidade_acoplamento + profundidade_base_acoplamento, True)

show_object(acoplamento)

w = cq.Workplane('XY', origin=(0, 0 ,espacamento_base_acoplamento)).moveTo(-raio_base_acoplamento_2, 0).threePointArc((0, -raio_base_acoplamento_2), (raio_base_acoplamento_2, 0))
w = w.lineTo(largura_braco / 2, largura_braco).lineTo(largura_braco / 2, comprimento_braco - comprimento_prender_ima)
p = comprimento_braco - comprimento_prender_ima
o = 1
for i in range(comprimento_prender_ima // (2*raio_ondulacao)):
    w = w.threePointArc((largura_braco / 2 + raio_ondulacao*o - raio_arredondamentos, p + (raio_ondulacao-o*raio_arredondamentos)), (largura_braco / 2, p + 2*(raio_ondulacao-o*raio_arredondamentos)))
    p += 2*(raio_ondulacao-o*raio_arredondamentos)
    o = -1 * o

w = w.threePointArc((0, comprimento_braco + largura_braco/2), (-largura_braco/2, comprimento_braco))

p = comprimento_braco
o = 1
for i in range(comprimento_prender_ima // (2*raio_ondulacao)):
    w = w.threePointArc((-largura_braco / 2 + raio_ondulacao*o + raio_arredondamentos, p - (raio_ondulacao+o*raio_arredondamentos)), (-largura_braco / 2, p - 2*(raio_ondulacao+o*raio_arredondamentos)))
    p -= 2*(raio_ondulacao+o*raio_arredondamentos)
    o = -1 * o

w = w.lineTo(-largura_braco / 2, largura_braco).lineTo(-raio_base_acoplamento_2, 0)
w = w.close().extrude(altura_braco)
#w = w.edges('#Z').fillet(raio_arredondamentos/2)
w = w.union(w.shell(raio_arredondamentos)).clean()#w.faces('|Z').edges().fillet(raio_arredondamentos)

w = w.union(acoplamento.full).clean()
w = w.cut(acoplamento.neg).clean()

#point = w.edges('>Z and %circle').vals()[0].arcCenter()

#w = w.edges(BoxSelector((point.x+profundidade_base_acoplamento,point.y+profundidade_base_acoplamento,point.z+profundidade_base_acoplamento), (point.x-profundidade_base_acoplamento,point.y-profundidade_base_acoplamento,point.z-profundidade_base_acoplamento)))
#w = w.fillet(raio_arredondamentos)

show_object(w)
if True:
    exporters.export(w, 'braco_2.stl')