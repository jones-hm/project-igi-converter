import os
import io
import struct


def read(File,Pth):

# ------------------Reading Lines From File
    
    FLINES = []

    with open(File, 'r') as FILE:

        for line in FILE:

         FLINES.append(line[:-1])

        FILE.close()


# ------------------ Reading Data From Lines


    if ( True ): 
        
        Data = []
    
        Header = [ "Anim Name","Bone Header","Bone Links","Bone Hierarchy","Anims Header","Anims List" ]

        for i in  range ( 0 , len(FLINES) ):

          if ( FLINES[i][:2] == "\\\\" ):

            assert Header[0] == FLINES[i][3:-1], "Header -> \""+str(Header[0])+"\" Not Found in Base File"

            for j in range ( i+1 , len(FLINES) ):

              if ( FLINES[j].find("[") != -1 ):

                for k in range ( j , len(FLINES) ):

                 if ( FLINES[k].find("]") != -1 ):

                  Data.append(FLINES[j:k+1])

                  break

                break

            del Header[0]
            


 # Conversion Strings of Data into Numbers/floats ------------->
 
        
        Num = [ "0","1","2","3","4","5","6","7","8","9" ]

        Float = [ "0","1","2","3","4","5","6","7","8","9",".","e","-","+" ]

        Str = [ "\"" ]

        End = [ " ",",","[","]","(",")" ]


        DATA = []

        for i in range ( 0 , len(Data) ):

            Par = []

            P = -1

            STR = str(Data[i])

            while ( P < len(STR)-1 ):

                P += 1

                if ( STR[P] in End ):

                    continue

                if ( STR[P] in Str ):

                   p = 0

                   while ( P+p < len(STR)-1 ):

                    p += 1   

                    if ( STR[P+p] in Str ):

                     Par.append(STR[P+1:P+p])

                     break
                
                   P += p
                                   
                if STR[P] in Float:

                   p = 0   

                   while ( P+p < len(STR)-1 ):

                    p += 1 

                    if ( STR[P+p] in End ):

                      if ( "." in STR[P:P+p] ):  

                        Par.append(float(STR[P:P+p]))

                      if ( "." not in STR[P:P+p] ):  

                        Par.append(int(STR[P:P+p]))

                      break
                
                   P += p

            DATA.append(Par)


 # CHECK FOR REQUIRED FORMAT ---->
 
 
        if ( len(DATA) != 6  ):
                               assert 0 != 0 , "Wrong Data Format in Base File" 

        if ( len(DATA[0]) != 1  ):
                               assert 0 != 0 , "Wrong Data Format in Anim Name" 
 
        if ( len(DATA[1]) != 2  ):
                               assert 0 != 0 , "Wrong Data Format in Bone Header" 
 
        if ( len(DATA[2]) != DATA[1][1] ):
                               assert 0 != 0 , "Wrong Data Format in Bone Links" 
 
        if ( len(DATA[3]) != 3*DATA[1][1] ):
                               assert 0 != 0 , "Wrong Data Format in Bone Hierarchy" 
 
        if ( len(DATA[4]) != 2  ):
                               assert 0 != 0 , "Wrong Data Format in Anim Header" 
 
        if ( len(DATA[5]) != DATA[4][0] ):
                               assert 0 != 0 , "Wrong Data Format in Anims List " 


 # CHECK FOR FILE PRESENCE ---->

 
        for i in range ( 0 , len(DATA[5]) ):

            Afile = Pth + "\\" + DATA[5][i]

            if (os.path.exists(Afile)) == False :

             assert 0 != 0 , "Anim Not Found -> "+str(Afile)
             

             

    return DATA[1],[DATA[2]],[DATA[3]],DATA[4],[DATA[5]]
