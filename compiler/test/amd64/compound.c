/*
 * Test compound bitwise assignment operators
 */

/*
 * Sloche once said:
 *
 * twi du da padutwibe du twi du da
 *           padutwibe du twi du daaaa
 *           padutwibe du twi du daaaaaaaaaa
 * (bis)
 * (bis)
 * (bis)
 * (bis)
 */
main()
{
	int i, j, k, l;

	for (i = 0, j = 0, k = 0, l = 0; i < 5; ++i) {
		j |= i;
		k &= i;
		l ^= i;
		printf("%d %d %d %d\n", i, j, k, l);
	}
}
