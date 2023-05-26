import cadquery as cq
from types import SimpleNamespace
from cadquery import exporters

def motor_1_dim():
    ret = SimpleNamespace()
    
    motor_1_raio = 41.9 / 2
    motor_1_altura = 18.9
    # 19.55 - 18.9
    motor_1_altura_engrenagem = 10
    # medicoes acoplamento (não utilizadas) 0.8, 5.3 (4.5 de nada)
    # raio_arre = 4 (não utilizado, seria para a aba)
    # coisa -> 3mm altura, 13.5 largura, 5; durante 12 mm com fio embaixo, 4.3 de altura, diametro 27.7
    motor_1_coisa_raio = 27.7 / 2
    motor_1_coisa_altura = 3
    motor_1_coisa_abaixo = 19.55 - motor_1_altura
    motor_1_coisa_largura = 13.5
    motor_1_coisa_alem_raio = 5
    motor_1_coisa_fios_comprimento = 12
    motor_1_coisa_fios_altura = 4.3 
    
    ret.raio = motor_1_raio
    ret.altura = motor_1_altura
    
    ret.altura_engrenagem = motor_1_altura_engrenagem
    
    ret.coisa_raio = motor_1_coisa_raio
    ret.coisa_altura = motor_1_coisa_altura
    ret.coisa_abaixo = motor_1_coisa_abaixo
    ret.coisa_largura = motor_1_coisa_largura
    ret.coisa_alem_raio = motor_1_coisa_alem_raio
    ret.coisa_fios_comprimento = motor_1_coisa_fios_comprimento
    ret.coisa_fios_altura = motor_1_coisa_fios_altura 
    
    return ret
    
def motor_1_suporte(where=(0, 0, 0), rotate=False, rotation_axis=((0, 0, 0), (0, 0, 0)), rotation=0):
    
    dim = motor_1_dim()
    espessura = 2.8
    folga_vertical = 2
    raio_base = 50
    arredondamentos = 0.8
    altura_max = (dim.altura + dim.coisa_abaixo) - folga_vertical + espessura
    
    #ret = cq.Workplane('XY').circle(dim.raio + espessura).extrude(dim.altura - folga_vertical)
    ret = (cq.Workplane('YZ').moveTo(0, 0).lineTo(raio_base - arredondamentos, 0)
          .radiusArc((raio_base, arredondamentos), -arredondamentos).lineTo(raio_base, espessura - arredondamentos)
          .radiusArc((raio_base-arredondamentos, espessura), -arredondamentos)
          .lineTo(dim.raio + espessura + 12, espessura)
          .tangentArcPoint((dim.raio + espessura, espessura + 7), relative=False)
          .lineTo(dim.raio + espessura, altura_max)
          .lineTo(dim.raio, altura_max)
          .lineTo(dim.raio, espessura + dim.coisa_abaixo)
          .lineTo(dim.coisa_raio, espessura + dim.coisa_abaixo)
          .lineTo(dim.coisa_raio, espessura)
          .lineTo(0, espessura).lineTo(0, 0).close()
          .revolve())
    ret = ret.cut(cq.Workplane('XY').moveTo(dim.coisa_raio-2, 0)
                  .rect(8*dim.coisa_fios_comprimento, dim.coisa_largura, centered=(False, True))
                  .extrude(12*dim.coisa_fios_altura)
                  .translate((0, 0, espessura - dim.coisa_fios_altura + dim.coisa_altura))).clean()
    
    if rotate:
        ret = ret.rotate(*rotation_axis, rotation)
    ret = ret.translate(where)
    
    ret.dim = dim
    
    return ret
    

def motor_1(where=(0, 0, 0), rotate=False, rotation_axis=((0, 0, 0), (0, 0, 0)), rotation=0):
    
    ret = cq.Workplane('XY')
    if rotate:
        ret = ret.rotate(*rotation_axis, rotation)
    ret = ret.translate(where)
    
    ret.dim = motor_1_dim()
    
    return ret
    
    
#raise Exception(f"{__name__}")
if __name__ == "temp":
    obj = motor_1_suporte()
    show_object(obj)
    if False:
        exporters.export(obj, 'suporte_motor_1.stl')
    