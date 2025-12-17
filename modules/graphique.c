/* graphique.c
   simple fonctions d'affichage / statistique réutilisables
#include <stdio.h>


#include <stdio.h>
#include <math.h>
#include <limits.h>

void print_stats(long long total, const int *counts, int nb_classes)
{
	int min = INT_MAX;
	int max = 0;
	int zero_count = 0;

	double mean = (double)total / nb_classes;
	double variance = 0.0;
	double chi2 = 0.0;

	/* Parcours des classes 
	for (int i = 0; i < nb_classes; i++) {
		int c = counts[i];

		if (c == 0)
			zero_count++;

		if (c < min)
			min = c;
		if (c > max)
			max = c;

		double diff = c - mean;
		variance += diff * diff;

		if (mean > 0.0)
			chi2 += (diff * diff) / mean;
	}

	variance /= nb_classes;
	double stddev = sqrt(variance);
	double cv = (mean > 0.0) ? stddev / mean : 0.0;

	double largeur = (max > 0) ? ((double)(max - min) / (double)max) * 100.0 : 0.0;

	/* Affichage 
	printf("Total tirages comptés : %lld\n", total);
	printf("Nombre de classes     : %d\n", nb_classes);
	printf("Classes vides         : %d\n", zero_count);
	printf("Min occurrences       : %d\n", min);
	printf("Max occurrences       : %d\n", max);

	printf("Largeur naive         : %.6f %%\n", largeur);

	printf("Moyenne par classe    : %.2f\n", mean);
	printf("Écart-type            : %.4f\n", stddev);
	printf("Coefficient variation : %.4f (%.2f %%)\n", cv, cv * 100.0);

	printf("Chi²                  : %.2f\n", chi2);
	printf("Degrés de liberté     : %d\n", nb_classes - 1);
}

*/