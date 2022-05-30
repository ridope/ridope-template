#include "main.h"


uint8_t ctr[TC_AES_BLOCK_SIZE];

/*-----------------------------------------------------------------------*/
/* Uart                                                                  */
/*-----------------------------------------------------------------------*/

static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if(readchar_nonblock()) {
		c[0] = getchar();
		c[1] = 0;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				fputs("\n", stdout);
				ptr = 0;
				return s;
			default:
				if(ptr >= (sizeof(s) - 1))
					break;
				fputs(c, stdout);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}

	return NULL;
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}

static void prompt(void)
{
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/

static void help(void)
{
	puts("\nLiteX minimal demo app built "__DATE__" "__TIME__"\n");
	puts("Available commands:");
	puts("help               - Show this command");
	puts("reboot             - Reboot CPU");
	puts("encrypt            - Encrypts a text");
	puts("decrypt            - Decrypts to a text");
}

/*-----------------------------------------------------------------------*/
/* Commands                                                              */
/*-----------------------------------------------------------------------*/

static void reboot_cmd(void)
{
	ctrl_reset_write(1);
}

static uint8_t get_hex_rep(char *str_key, uint8_t *hex_key)
{	
	if(str_key == NULL || hex_key == NULL)
	{
		printf("\e[91;1mNull pointers\e[0m\n");
		return 0;
	}

	int key_size = 0;

	char temp_str[3];
	temp_str[2] = '\0';

	for(int i = 0; i < 2*TC_AES_BLOCK_SIZE; i+=2)
	{
		if(str_key[i]==0 || str_key[i+1]==0)
		{
			break;
		}

		temp_str[0] = str_key[i];
		temp_str[1] = str_key[i+1];

		hex_key[key_size] = strtol(&temp_str[0],NULL,16);

		key_size++;
	}

	if(key_size != TC_AES_BLOCK_SIZE){
		printf("\e[91;1mKey size: %d\e[0m\n", key_size);
		return 0;
	}

	return 1;
}

static uint8_t str_to_aes_block(char *str_text, uint8_t *text_block)
{
	if(str_text == NULL || text_block == NULL)
	{
		printf("\e[91;1mNull pointers\e[0m\n");
		return 0;
	}

	memset(text_block, 0, TC_AES_BLOCK_SIZE);

	if(snprintf((char *)text_block, TC_AES_BLOCK_SIZE, "%s", str_text) < 0){
		printf("\e[91;1mEncoding error\e[0m\n");
		return 0;
	}

	return 1;
}

static void encrypts(uint8_t *counter, uint8_t len_counter)
{	
	char *str;
	char *key;
	char *text;

	uint8_t nist_key[TC_AES_BLOCK_SIZE];

	printf("\e[94;1mInsert the key\e[0m> ");
	do 
	{
		str = readstr();
	}while(str == NULL);

	key = get_token(&str);

	if (get_hex_rep(key, &nist_key[0]) == 0){
		printf("\e[91;1mError converting the encryption key\e[0m\n");
		return;
	}

	printf("\e[94;1mType the text\e[0m> ");
	do 
	{
		str = readstr();
	}while(str == NULL);

	text = get_token(&str);

	uint8_t chiper_size = TC_AES_BLOCK_SIZE+strlen(text);
	uint8_t *ciphertext = malloc(chiper_size);

	struct tc_aes_key_sched_struct s;

	if (tc_aes128_set_encrypt_key(&s, nist_key) == 0){
		printf("\e[91;1mError setting the encryption key\e[0m\n");
	}

	(void)memcpy(ciphertext, counter, len_counter);
	
	if (tc_ctr_mode(&ciphertext[TC_AES_BLOCK_SIZE], strlen(text), (uint8_t *) text, strlen(text), counter, &s) == 0) {
			printf("\e[91;1mError in the text encryption\e[0m\n");
	}

	printf("\e[94;1mChiper text: \e[0m");
	for(int i=0; i < chiper_size; i++)
	{
		printf("%02x", ciphertext[i]);
	}

	printf("\n");

}

static void decrypts(void)
{
	char *str;
	char *key;
	char *text;

	uint8_t nist_key[TC_AES_BLOCK_SIZE];
	uint8_t ciphertext[TC_AES_BLOCK_SIZE];
	uint8_t text_out[TC_AES_BLOCK_SIZE];

	printf("\e[94;1mInsert the key\e[0m> ");
	do 
	{
		str = readstr();
	}while(str == NULL);

	key = get_token(&str);

	if (get_hex_rep(key, &nist_key[0]) == 0){
		printf("\e[91;1mError converting the encryption key\e[0m\n");
		return;
	}

	printf("\e[94;1mInsert the chipertext\e[0m> ");
	do 
	{
		str = readstr();
	}while(str == NULL);

	text = get_token(&str);

	if (get_hex_rep(text, &ciphertext[0]) == 0){
		printf("\e[91;1mError converting the ciphertext\e[0m\n");
		return;
	}

	struct tc_aes_key_sched_struct s;

	if (tc_aes128_set_decrypt_key(&s, nist_key) == 0){
		printf("\e[91;1mError setting the decryption key\e[0m\n");
	}

	if (tc_aes_decrypt(text_out, ciphertext, &s) == 0){
		printf("\e[91;1mError in the text decryption\e[0m\n");
	}
	
	printf("\e[94;1mText: \e[0m");
	printf("%s\n", text_out);
}


/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/

static void console_service(void)
{
	char *str;
	char *token;

	str = readstr();
	if(str == NULL) return;
	token = get_token(&str);
	if(strcmp(token, "help") == 0)
		help();
	else if(strcmp(token, "reboot") == 0)
		reboot_cmd();

	else if(strcmp(token, "encrypt") == 0)
		encrypts(ctr, TC_AES_BLOCK_SIZE);
	
	else if(strcmp(token, "decrypt") == 0)
		decrypts();

	prompt();
}


int main(void)
{
	#ifdef CONFIG_CPU_HAS_INTERRUPT
		irq_setmask(0);
		irq_setie(1);
	#endif
	uart_init();

	help();
	prompt();


	/* Generating Counter init */
	for(int i=0; i< TC_AES_BLOCK_SIZE; i++)
	{
		ctr[i] = rand() % 255;
	}

	while(1) {
		console_service();
	}

	return 0;
}
