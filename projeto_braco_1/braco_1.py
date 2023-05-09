import cadquery as cq
import math
from cadquery import exporters
from motor_2 import motor_2
import teardrop  # Adds the teardrop function to cadquery.Workplane

# em milímetros
comprimento_braco = 120
largura_braco = 12
altura_braco = 10
profundidade_acoplamento = 7.2

raio_base_acoplamento = 12
raio_base_motor = 15


raio_roda = 23 / 2
largura_roda = 6.4
eixo_roda_total = 6.2
eixo_roda_1 = 1.5
eixo_roda_2 = 3
raio_eixo_roda_1 = 4.6 / 2
raio_eixo_roda_2 = 2.5 / 2
altura_hexagono_roda = 1.71
raio_furo_eixo = 2 / 2

altura_motor = 34.6
tolerancia_altura_roda = 0.8

dist_suporte_roda = 20
raio_menor_suporte_roda = 5
altura_suporte_roda = altura_motor - raio_roda + altura_braco
largura_suporte_roda = 8
outra_largura_suporte_roda = 10

raio_arrendondamentos = 1.6

w = cq.Workplane('XY').box(comprimento_braco,
                           largura_braco,
                           altura_braco,
                           (False, True, False))

#w = w.edges('|X').fillet(1)
wc = cq.Workplane('XY').moveTo(comprimento_braco, 0).circle(raio_base_motor).extrude(altura_braco, combine=False)
wc = wc.edges().fillet(raio_arrendondamentos)
w = w.edges('|X').fillet(raio_arrendondamentos)
w = w.union(wc).clean()

altura_ate_furo_roda = altura_braco - profundidade_acoplamento + altura_motor - raio_roda + tolerancia_altura_roda
w_roda = cq.Workplane('YZ', (comprimento_braco - dist_suporte_roda + largura_suporte_roda / 2 + largura_roda / 2, 0, altura_ate_furo_roda)).circle(raio_roda).extrude(largura_roda / 2, both=True).edges().fillet(1.0).faces('<X').workplane().tag('sel').polygon(6, altura_hexagono_roda).extrude(eixo_roda_total).workplaneFromTagged('sel').circle(raio_eixo_roda_2).extrude(eixo_roda_2).workplaneFromTagged('sel').circle(raio_eixo_roda_1).extrude(eixo_roda_1)

w_sup = (cq.Workplane('YZ', (comprimento_braco - dist_suporte_roda, 0, 0))
    .moveTo(-outra_largura_suporte_roda / 2, 0)
    .lineTo(outra_largura_suporte_roda / 2, 0)
    .lineTo(outra_largura_suporte_roda / 2, altura_ate_furo_roda)
    .threePointArc((0, altura_ate_furo_roda + raio_menor_suporte_roda), (-outra_largura_suporte_roda / 2, altura_ate_furo_roda))
    .lineTo(-outra_largura_suporte_roda / 2, 0)
    .close()
    .extrude(largura_suporte_roda / 2, both=True).edges('(not >Z) and |X').fillet(raio_arrendondamentos).faces('|X').edges().fillet(raio_arrendondamentos))

#.polyline(pts).close()
w_sup.plane.origin = (comprimento_braco - dist_suporte_roda + largura_suporte_roda / 2, 0, altura_ate_furo_roda)
w_sup = (w_sup.transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
         .teardrop(raio_furo_eixo).cutThruAll().transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
         .teardrop(raio_eixo_roda_1+0.02).cutBlind(-eixo_roda_1).transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
         .teardrop(raio_eixo_roda_2+0.02).cutBlind(-eixo_roda_2))

w_sup.plane.origin = (comprimento_braco - dist_suporte_roda - largura_suporte_roda / 2, 0, altura_ate_furo_roda)
w_sup = (w_sup.transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
         .teardrop(raio_eixo_roda_1+0.02).cutBlind(eixo_roda_1).transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
         .teardrop(raio_eixo_roda_2+0.02).cutBlind(eixo_roda_2))
         #.workplaneFromTagged('eixo').teardrop(raio_eixo_roda_2).cutBlind(0))
#w = w.edges('|X').fillet(raio_arrendondamentos)
#w = w.edges('|Z').fillet(raio_arrendondamentos)
#w = w.edges('|Y').fillet(raio_arrendondamentos)
w = w.union(w_sup).clean()
w = w.edges('|Y').fillet(raio_arrendondamentos)

w_motor = motor_2((comprimento_braco, 0.0, 0.0), rotate=True, rotation_axis=((0.0, -1.0, 0.0), (0.0, 1.0, 0.0)), rotation=180)

from testes_acoplamento import acoplamento
obj_acoplamento = acoplamento(profundidade_acoplamento, altura_braco, raio_base_acoplamento, raio_arrendondamentos)
neg = cq.Workplane('XY').circle(obj_acoplamento[1] - raio_arrendondamentos).extrude(altura_braco)
w = w.moveTo(0, 0).cut(neg).union(obj_acoplamento[0])

raio_menor_furos = 3 / 2
raio_maior_furos = 5 / 2
altura_furos = 5

w_furo = (cq.Workplane('YZ').moveTo(raio_maior_furos, 0)
          .radiusArc((raio_menor_furos, raio_menor_furos), raio_menor_furos)
          .lineTo(raio_menor_furos, altura_furos - raio_menor_furos)
          .radiusArc((0, altura_furos), -raio_menor_furos).lineTo(0, 0).close()
          .revolve())

p = comprimento_braco
r = 7
dist = [(p + r*math.cos(i), r*math.sin(i)) for i in [0, math.pi*2/3, math.pi*4/3]]
w = w.transformed((0, 180, 0), (0, 0, altura_braco)).pushPoints(dist).eachpoint(lambda f: w_furo.findSolid().moved(f), combine='cut')

#w_motor = w_motor.translate((0.0, 0.0, 2.0))
#w = w.cut(w_motor).clean()

#ra = motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte
#w_obj = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).circle(ra).extrude(-motor_2_altura - altura_braco - 5).translate((0.0, 0.0, altura_braco + 5))
#w_temp = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).rect(2*motor_2_raio_suporte, 2*ra + 4, True).extrude(-motor_2_altura - altura_braco - 5).translate((0.0, 0.0, altura_braco + 5))
#w_obj = w_obj.intersect(w_temp).clean().cut(w_motor).clean().cut(w_motor.shell(0.4)).clean()#.cut(w).clean().cut(w.shell(0.4)).clean()
#w_temp = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).circle(raio_base_motor + 0.4).extrude(-altura_braco - 55).faces().fillet(1.0).translate((0.0, 0.0, altura_braco))
#w_obj = w_obj.cut(w_temp).clean()
#w_subtemp = cq.Workplane('XY', origin=(comprimento_braco, 0.0, -motor_2_altura - 4)).sphere(ra)
#w_temp = cq.Workplane('XY', origin=(comprimento_braco, 0.0, 0.0)).pushPoints([(0.0, motor_2_raio + motor_2_dist_furo), (0.0, -motor_2_raio - motor_2_dist_furo)]).circle(motor_2_raio_furo).extrude(-motor_2_altura - 5).intersect(w_subtemp).clean()
#w_obj = w_obj.cut(w_temp).clean()

#show_object(w_motor)
show_object(w)
show_object(w_roda)
#show_object(w_temp)
#show_object(w_subtemp)
#show_object(w_obj)
show_object(w_motor)

if False:
    exporters.export(w, 'braco_1.stl')
    #exporters.export(w_obj, 'coisa.stl')
