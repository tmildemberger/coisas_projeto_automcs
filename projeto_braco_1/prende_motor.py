import cadquery as cq
from cq_warehouse.fastener import RaisedCheeseHeadScrew
import cq_warehouse.extensions
from cadquery import exporters

parafuso = RaisedCheeseHeadScrew(size="M3-1", fastener_type="iso7045",length=8, simple=False)

#show_object(parafuso)

#w = cq.Workplane('XY').box(2, 2, 2.2)

#show_object(w)

motor_2_raio = 14
motor_2_altura = 18
motor_2_dist_furo = 3.5
motor_2_raio_furo = 2
motor_2_raio_suporte = 3.5

comprimento_braco = 120
largura_braco = 16
altura_braco = 8
raio_base_motor = 15

w_motor = cq.Workplane('XY').moveTo(-motor_2_raio_suporte, 0).lineTo(-motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo).threePointArc((0, motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte), (motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo)).lineTo(motor_2_raio_suporte, 0).close().moveTo(0, motor_2_raio + motor_2_dist_furo).circle(motor_2_raio_furo).mirrorX().extrude(-0.8).translate((0.0, 0.0, motor_2_altura))
w_motor = w_motor.transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).circle(motor_2_raio).extrude(motor_2_altura).transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).box(15.5, 14.5, motor_2_altura, (False, True, False)).edges('>X and |Z').fillet(1.0).edges('<Z').fillet(1.0)
w_motor = w_motor.transformed((0.0, 0.0, 0.0), (-motor_2_raio + 6.5, 0.0, motor_2_altura)).circle(4.5).extrude(1.5).faces('>Z').workplane().circle(2.5).extrude(8).faces('>Z').workplane().pushPoints([(0.0, -3), (0.0, 3)]).rect(7, 3, True).extrude(-6, combine='cut')
w_motor.plane.origin = (0.0, 0.0, 0.0)
#w_motor = w_motor.rotate((0.0, -1.0, 0.0), (0.0, 1.0, 0.0), 180).translate((comprimento_braco, 0.0, 0.0))


ra = motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte
w_obj = cq.Workplane('XY').circle(ra).extrude(motor_2_altura + altura_braco + 5).translate((0.0, 0.0, -altura_braco - 5))
w_temp = cq.Workplane('XY').rect(2*motor_2_raio_suporte, 2*ra + 4, True).extrude(motor_2_altura + altura_braco + 5).translate((0.0, 0.0, -altura_braco - 5))
w_obj = w_obj.intersect(w_temp).clean().cut(w_motor).clean().cut(w_motor.shell(0.4)).clean()#.cut(w).clean().cut(w.shell(0.4)).clean()
w_temp = cq.Workplane('XY').circle(raio_base_motor + 0.4).extrude(altura_braco + 55).faces().fillet(1.0).translate((0.0, 0.0, -altura_braco))
w_obj = w_obj.cut(w_temp).clean()


hmm = cq.Assembly(None, name="hmm")

w_obj.plane.origin = (0.0, 0.0, motor_2_altura+2.4)
w_obj = w_obj.pushPoints([(0.0, motor_2_raio + motor_2_dist_furo), (0.0, -motor_2_raio - motor_2_dist_furo)]).threadedHole(fastener=parafuso, depth=10.4, baseAssembly=hmm)
w_obj.plane.origin = (0.0, 0.0, motor_2_altura)
w_obj = w_obj.pushPoints([(0.0, motor_2_raio + motor_2_dist_furo), (0.0, -motor_2_raio - motor_2_dist_furo)]).circle(motor_2_raio_furo).extrude(-1.2, combine='cut')
hmm.add(w_obj, name="w_obj", color=cq.Color(162 / 255, 138 / 255, 255 / 255))
#.circle(5.5).extrude(2)
show_object(hmm)
show_object(w_obj)
show_object(w_motor)

w_teste = cq.Workplane('XY', origin=(0.0, motor_2_raio + motor_2_dist_furo, motor_2_altura)).rect(7, 7, True).extrude(-10)
w_teste = w_obj.intersect(w_teste).clean()
show_object(w_teste)

if True:
    exporters.export(w_obj, 'coisa.stl')
if True:
    exporters.export(w_teste, 'teste_coisa.stl')