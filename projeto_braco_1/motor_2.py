import cadquery as cq
from types import SimpleNamespace

def motor_2_dim():
    ret = SimpleNamespace()
    
    motor_2_raio = 14
    motor_2_altura = 19
    motor_2_dist_furo = 3.5
    motor_2_raio_furo = 2
    motor_2_raio_suporte = 3.5
    
    ret.raio = motor_2_raio
    ret.altura = motor_2_altura
    ret.dist_furo = motor_2_dist_furo
    ret.raio_furo = motor_2_raio_furo
    ret.raio_suporte = motor_2_raio_suporte
    return ret

def motor_2(where=(0, 0, 0), rotate=False, rotation_axis=((0, 0, 0), (0, 0, 0)), rotation=0):
    
    motor_2_raio = 14
    motor_2_altura = 19
    motor_2_dist_furo = 3.5
    motor_2_raio_furo = 2
    motor_2_raio_suporte = 3.5
    
    w_motor = cq.Workplane('XY').moveTo(-motor_2_raio_suporte, 0).lineTo(-motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo).threePointArc((0, motor_2_raio + motor_2_dist_furo + motor_2_raio_suporte), (motor_2_raio_suporte, motor_2_raio + motor_2_dist_furo)).lineTo(motor_2_raio_suporte, 0).close().moveTo(0, motor_2_raio + motor_2_dist_furo).circle(motor_2_raio_furo).mirrorX().extrude(-0.8).translate((0.0, 0.0, motor_2_altura))
    w_motor = w_motor.transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).circle(motor_2_raio).extrude(motor_2_altura).transformed((0.0, 0.0, 0.0), (0.0, 0.0, 0.0)).box(15.5, 14.5, motor_2_altura, (False, True, False)).edges('>X and |Z').fillet(1.0).edges('<Z').fillet(1.0)
    w_motor = w_motor.transformed((0.0, 0.0, 0.0), (-motor_2_raio + 6.5, 0.0, motor_2_altura)).circle(4.5).extrude(1.5).faces('>Z').workplane().circle(2.5).extrude(8).faces('>Z').workplane().pushPoints([(0.0, -3), (0.0, 3)]).rect(7, 3, True).extrude(-6, combine='cut')
    w_motor.plane.origin = (0.0, 0.0, 0.0)
    
    ret = w_motor
    if rotate:
        ret = ret.rotate(*rotation_axis, rotation)
    ret = ret.translate(where)
    
    ret.raio = motor_2_raio
    ret.altura = motor_2_altura
    ret.dist_furo = motor_2_dist_furo
    ret.raio_furo = motor_2_raio_furo
    ret.raio_suporte = motor_2_raio_suporte
    
    return ret
    
    
#raise Exception(f"{__name__}")
if __name__ == "temp":
    obj = motor_2()
    show_object(obj)
    