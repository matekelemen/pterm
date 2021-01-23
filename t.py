from colors.colors import _color_code, color

print( _color_code((255,0,0), 30) )

for character in color( "wtf", (255,0,0) ):
    print( character, end="." )

print("")