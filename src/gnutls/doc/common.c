char *escape_string(const char *str, char *buffer, int buffer_size)
{
	int i = 0, j = 0;


	while (str[i] != 0 && j < buffer_size - 1) {
		if (str[i] == '_') {
			buffer[j++] = '\\';
			buffer[j++] = '_';
			buffer[j++] = '\\';
			buffer[j++] = '-';
		} else if (str[i] == '#') {
			buffer[j++] = '\\';
			buffer[j++] = '#';
		} else {
			buffer[j++] = str[i];
		}
		i++;
	};

	buffer[j] = 0;

	return buffer;

}

char *escape_texi_string(const char *str, char *buffer, int buffer_size)
{
	int i = 0, j = 0;


	while (str[i] != 0 && j < buffer_size - 1) {
		if (str[i] == '_') {
			buffer[j++] = '_';
			buffer[j++] = '@';
			buffer[j++] = '-';
		} else {
			buffer[j++] = str[i];
		}
		i++;
	};

	buffer[j] = 0;

	return buffer;

}
