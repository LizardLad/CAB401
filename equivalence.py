#This can verify the equivalence of the two implementations

parallel_filepath = open("parallel.vocab", "rb")
serial_filepath = open("serial.vocab", "rb")

parallel_output = parallel_filepath.read()
serial_output = serial_filepath.read()

if parallel_output == serial_output:
    print("The two implementations are equivalent")
else:
    print("The two implementations are not equivalent")