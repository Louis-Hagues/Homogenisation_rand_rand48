# **Comment compiler ce projet**
Avant d'expliquer quel est ce projet, voici la ligne pour compiler ce projet : #\n
gcc -O2 -std=c11 \ -o mc_main \ modules/main.c \ modules/programme/traitement.c \ modules/programme/socket.c \ -lpthread -lrt -lm

Pour le lancer il faudra tout simplement taper :
./mc_main

---

# 🌐 **Résumé global du projet – Niveau 2 : deux PC en coopération**

---


## 🎯 **Objectif**

Mettre en place un **programme distribué en C** qui teste la **répartition et la qualité d’un générateur aléatoire** (`rand()`, `random()`, etc.)
sur **toute la plage possible** `[0, RAND_MAX]`,
en **divisant le travail entre deux ordinateurs** pour **doubler la vitesse d’exécution**.

---

## 🖥️ **Principe général**

Le calcul est partagé entre :

* **PC1 (serveur principal)**
  → Coordonne le test, fait sa moitié des tirages, récupère les résultats du second PC et effectue la fusion finale.
* **PC2 (client distant)**
  → Reçoit sa portion de la plage à traiter, fait sa partie du calcul, puis renvoie les résultats à PC1.

Les deux machines réalisent **le même type de travail localement** (génération aléatoire, comptage, mémoire partagée),
mais elles communiquent entre elles via une **connexion réseau TCP/IP**.

---

## ⚙️ **Déroulement du programme**

### 🟢 Étape 1 — Lancement du serveur (PC1)

* Démarre en **mode serveur TCP**.
* Attend la connexion du client.
* Une fois connecté, il :

  * Envoie à PC2 les paramètres du test (bornes de la plage, nombre de tirages à effectuer, etc.).
  * Lance ses **propres processus locaux** pour traiter la première moitié de la plage (`0 → RAND_MAX / 2`).

### 🔵 Étape 2 — Lancement du client (PC2)

* Démarre en **mode client TCP** et se connecte à PC1 via son adresse IP.
* Reçoit la **plage de valeurs** à traiter (par exemple `RAND_MAX / 2 + 1 → RAND_MAX`).
* Exécute sa partie du travail localement :

  * Crée plusieurs **processus fils** avec `fork()`.
  * Utilise une **mémoire partagée locale (IPC)** pour stocker les compteurs.
  * Réalise ses tirages aléatoires et remplit son tableau de fréquences.

### 🔁 Étape 3 — Échanges de données

* Une fois son travail terminé, PC2 envoie son **tableau de résultats** à PC1 par la **socket TCP**.
* Les données envoyées peuvent être :

  * Un **tableau complet** (si la mémoire et le réseau le permettent),
  * Ou un **ensemble de “buckets” agrégés** (par blocs de valeurs).
* PC1 reçoit ces données et les stocke dans un tableau global.

### ⚙️ Étape 4 — Fusion et analyse

* PC1 additionne ses propres résultats et ceux reçus de PC2 pour obtenir la **distribution complète sur `[0, RAND_MAX]`**.
* Il calcule ensuite :

  * La **fréquence moyenne** des tirages,
  * La **variance et l’écart-type**,
  * Les **valeurs minimales et maximales**,
  * Et éventuellement un **test d’uniformité** (comme le χ²).
* Les résultats sont affichés ou enregistrés pour analyse.

### 🧹 Étape 5 — Nettoyage

* Les deux machines libèrent leurs ressources locales :

  * Détachement et suppression de la mémoire partagée.
  * Fermeture des sockets réseau.
* Le serveur met fin à la session après confirmation du client.

---

## 📊 **Répartition du travail**

| Ordinateur        | Rôle                             | Plage de valeurs traitée        | Tirages effectués |
| ----------------- | -------------------------------- | ------------------------------- | ----------------- |
| **PC1 (serveur)** | Calcule + fusionne les résultats | 0 → `RAND_MAX / 2`              | `N_TOTAL / 2`     |
| **PC2 (client)**  | Calcule et envoie ses résultats  | `RAND_MAX / 2 + 1` → `RAND_MAX` | `N_TOTAL / 2`     |

---

## 🔗 **Communication réseau**

* **Protocole :** TCP/IP (connexion fiable entre les deux machines).
* **Sens principal des données :**

  * Serveur → Client : paramètres du calcul.
  * Client → Serveur : tableau de résultats.
* **Réseau local recommandé** pour éviter les délais.
* Les données peuvent être envoyées **en binaire brut** pour maximiser la vitesse.

---

## 🧱 **Architecture logique**

```
                 ┌──────────────────────────────┐
                 │         PC1 (Serveur)        │
                 │------------------------------│
                 │ - Crée une socket TCP        │
                 │ - Envoie la plage à PC2      │
                 │ - Fait sa propre moitié      │
                 │ - Reçoit résultats du client │
                 │ - Fusionne et analyse        │
                 └──────────┬───────────────────┘
                            │
                     Connexion TCP/IP
                            │
                            ▼
                 ┌──────────────────────────────┐
                 │         PC2 (Client)         │
                 │------------------------------│
                 │ - Se connecte au serveur     │
                 │ - Reçoit sa plage            │
                 │ - Fait sa propre moitié      │
                 │ - Envoie ses résultats       │
                 └──────────────────────────────┘
```

---

## ✅ **Résultat final**

À la fin du calcul :

* Le **serveur (PC1)** dispose de la **distribution complète** du générateur aléatoire sur la totalité de `[0, RAND_MAX]`.
* Le **temps total d’exécution** est presque **divisé par deux**, car les deux PC travaillent en parallèle.
* Les **résultats** permettent d’évaluer :

  * L’uniformité du générateur,
  * La régularité de la distribution,
  * Et les éventuels biais ou anomalies sur certaines plages.

---

## ⚡ **Avantages**

✅ Calcul réellement **distribué et parallèle**.
✅ Communication réseau simple et standard (sockets TCP).
✅ Architecture **scalable** (facile à étendre à plus de PC plus tard).
✅ Fusion centralisée des résultats sur une seule machine (PC1).

---

## ⚠️ **Points de vigilance**

* La **mémoire partagée** reste **locale à chaque PC** — elle ne circule pas sur le réseau.
* Il faut **envoyer les résultats agrégés** (ou par blocs) si la taille des données est trop importante.
* Bien gérer :

  * Les **déconnexions éventuelles**,
  * Les **différences de vitesse** entre les deux machines,
  * Les **permissions réseau et pare-feu**.

---

## 🧩 **En résumé**

| Élément            | Description                                          |
| ------------------ | ---------------------------------------------------- |
| Nombre de machines | 2                                                    |
| Communication      | TCP/IP via sockets                                   |
| Rôle du serveur    | Coordonne, calcule, reçoit, fusionne                 |
| Rôle du client     | Calcule et envoie ses résultats                      |
| Travail total      | Plage complète `[0, RAND_MAX]` divisée en deux       |
| Mémoire partagée   | Locale à chaque machine                              |
| Synchronisation    | Par protocole réseau                                 |
| Objectif final     | Tester l’uniformité complète du générateur aléatoire |

---

Souhaites-tu que je te fasse une **représentation graphique** du fonctionnement entre les deux PC (avec étapes numérotées) à mettre dans ton rapport ?
Ce serait parfait pour illustrer cette architecture distribuée.
