
BLENDER="C:\Program Files\Blender Foundation\Blender 2.90\blender.exe"
DIST=../dist

all : \
    $(DIST)/sewer.pnct \
    $(DIST)/sewer.scene \

$(DIST)/sewer.scene : sewer.blend export-scene.py
    $(BLENDER) --background --python export-scene.py -- "sewer.blend:Main" "$(DIST)/sewer.scene"

$(DIST)/sewer.pnct : sewer.blend export-meshes.py
    $(BLENDER) --background --python export-meshes.py -- "sewer.blend:Main" "$(DIST)/sewer.pnct" 
