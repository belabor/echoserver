unsigned int sp_input(char* data) {
  /* check input */
  return 0x11;
}

unsigned int sp_encrypt_data() {
  sp_input();
  if sp_failover() {
    /* a thing happens */
  };
  return 0x44;
}

unsigned int sp_failover() {
  /* shutdown */
  return 0xFF;
}
