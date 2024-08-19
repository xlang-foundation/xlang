
# usage of bytes() function to initialize zero bytes objects
zero_len_bytes = bytes()
zero_len_bytes += bytes([12,34])
# usage of bytes() function to initialize bytes objects with size
bytes_with_size = bytes(10)
# usage of bytes() function to initialize bytes objects with list
# item values should be in range(0, 256)
bin_from_list = bytes([1, 2, 3, 4, 5])
# usage of bytes() function to do serialization for all other cases
bin_from_list_Serialization = bytes([1, 2, 3, 4, 5],Serialization = True)
x0 ="this is a test string"
print(bytes(x0))
x = ["abc",1,2,{'x':1,'y':2}]
y = bytes(x)
print(y,"\n",x)