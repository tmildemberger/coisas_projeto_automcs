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
outra_largura_suporte_roda = 10

raio_arrendondamentos = 1.6

motor_2_raio = 14
motor_2_altura = 18
motor_2_dist_furo = 3.5
motor_2_raio_furo = 2
motor_2_raio_suporte = 3.5

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
    (-outra_largura_suporte_roda / 2, 0),
    (outra_largura_suporte_roda / 2, 0),
    (outra_largura_suporte_roda / 2, h_max - outra_largura_suporte_roda / 2),
    (0, h_max),
    (-outra_largura_suporte_roda / 2, h_max - outra_largura_suporte_roda / 2),
    (-outra_largura_suporte_roda / 2, 0)
]
w_sup = cq.Workplane('YZ', (comprimento_braco - dist_suporte_roda, 0, 0)).polyline(pts).close().extrude(largura_suporte_roda / 2, both=True).edges('(not >Z) and |X').fillet(raio_arrendondamentos).edges('>Z').fillet(raio_menor_suporte_roda).faces('|X').edges().fillet(raio_arrendondamentos)
w_sup = w_sup.moveTo(0, altura_ate_furo_roda).teardrop(raio_furo_eixo).cutThruAll()
#w = w.edges('|X').fillet(raio_arrendondamentos)
#w = w.edges('|Z').fillet(raio_arrendondamentos)
#w = w.edges('|Y').fillet(raio_arrendondamentos)
w = w.union(w_sup).clean()
w = w.edges('|Y').fillet(raio_arrendondamentos)
from cadquery.occ_impl.geom import Vector, Plane, Location

w_motor = cq.Workplane('XY').moveTo(-motor_2_raio_suporte, 0).lineTo(-motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo).threePointArc((0, motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte), (motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo)).lineTo(motor_2_raio_suporte, 0).close().moveTo(0, motor_2_raio + motor_2_dist_furo).circle(motor_2_raio_furo).mirrorX().extrude(-0.8).translate((0.0, 0.0, motor_2_altura))
w_motor = w_motor.transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).circle(motor_2_raio).extrude(motor_2_altura).transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).box(15.5, 14.5, motor_2_altura, (False, True, False)).edges('>X and |Z').fillet(1.0).edges('<Z').fillet(1.0)
w_motor = w_motor.transformed((0.0, 0.0, 0.0), (-motor_2_raio + 6.5, 0.0, motor_2_altura)).circle(4.5).extrude(1.5).faces('>Z').workplane().circle(2.5).extrude(8).faces('>Z').workplane().pushPoints([(0.0, -3), (0.0, 3)]).rect(7, 3, True).extrude(-6, combine='cut')
w_motor.plane.origin = (0.0, 0.0, 0.0)
w_motor = w_motor.rotate((0.0, -1.0, 0.0), (0.0, 1.0, 0.0), 180).translate((comprimento_braco, 0.0, 0.0))

from testes_acoplamento import acoplamento
obj_acoplamento = acoplamento(profundidade_acoplamento, altura_braco, raio_base_acoplamento, raio_arrendondamentos)
neg = cq.Workplane('XY').circle(obj_acoplamento[1] - raio_arrendondamentos).extrude(altura_braco)
w = w.moveTo(0, 0).cut(neg).union(obj_acoplamento[0])

#w_motor = w_motor.translate((0.0, 0.0, 2.0))
#w = w.cut(w_motor).clean()

ra = motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte
w_obj = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).circle(ra).extrude(-motor_2_altura - altura_braco - 5).translate((0.0, 0.0, altura_braco + 5))
w_temp = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).rect(2*motor_2_raio_suporte, 2*ra + 4, True).extrude(-motor_2_altura - altura_braco - 5).translate((0.0, 0.0, altura_braco + 5))
w_obj = w_obj.intersect(w_temp).clean().cut(w_motor).clean().cut(w_motor.shell(0.4)).clean()#.cut(w).clean().cut(w.shell(0.4)).clean()
w_temp = cq.Workplane('XY').moveTo(comprimento_braco, 0.0).circle(raio_base_motor + 0.4).extrude(-altura_braco - 55).faces().fillet(1.0).translate((0.0, 0.0, altura_braco))
w_obj = w_obj.cut(w_temp).clean()
w_subtemp = cq.Workplane('XY', origin=(comprimento_braco, 0.0, -motor_2_altura - 4)).sphere(ra)
w_temp = cq.Workplane('XY', origin=(comprimento_braco, 0.0, 0.0)).pushPoints([(0.0, motor_2_raio + motor_2_dist_furo), (0.0, -motor_2_raio - motor_2_dist_furo)]).circle(motor_2_raio_furo).extrude(-motor_2_altura - 5).intersect(w_subtemp).clean()
w_obj = w_obj.cut(w_temp).clean()

#show_object(w_motor)
show_object(w)
show_object(w_roda)
#show_object(w_temp)
#show_object(w_subtemp)
show_object(w_obj)
show_object(w_motor)

if False:
    #exporters.export(w, 'braco_1.stl')
    exporters.export(w_obj, 'coisa.stl')
