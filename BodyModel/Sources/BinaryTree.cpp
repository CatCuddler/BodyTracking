#include "pch.h"
#include "BinaryTree.h"

#include <Kore/Log.h>

using namespace Kore;

BinaryTree::BinaryTree() {
	// Initialize array with nodes
	nodes = new Node*[numOfNodes];
	for (int i = 0; i < numOfNodes; ++i) {
		nodes[i] = nullptr;
	}
	
	const char* const introText = "Deine Karriere läuft gut. Du arbeitest im Zirkus - ein Traum, den du schon von Kindheit an verfolgst. Vor Kurzen kam es noch besser: Der Zirkusdirektor erzählt di, du seist die Hauptattraktion! Dein Gesicht ist auf allen Plakaten zu sehen! Du bist ein Star und reist mit dem Zirkus um die ganze Welt. Bis es zu einem Fürchterlichen Unfall kommt. Bei einer Abendvorstellung reißt ein Seil deines Trapezes und du stürzt. Wie durch ein Wunder kommst du ohne größere Verletzungen davon. Aber deine Karriere ist erst einmal auf Eis gelegt.Du glaubst nicht daran, dass es ein Unfall war. Dein Assistent hatte die Seile Extra noch kontrolliert! Du machst dich auf die Suche nach dem Täter.";
	const char* const outroText = "Der Assisstent des Magiers wurde nach seinem Geständnis von der Polizei abgeführt und aus der Community des Zirkus ́ verstoßen, nachdem herauskam, dass auch er es war, der Schnuffels lieblingsspielzeug geklaut hatte. So bekam auch Schnuffel seinen geliebtes Spielzeug wieder und lässt es seitdem nicht mehr aus den Augen. Nach einiger Zeit kehrte wieder Normalität in den Zirkusalltag ein. Nachdem deine Verletzungen geheilt waren, konntest du deine Karriere Fortsetzen. Das neue Zirkusplakat musst du dir nun allerdings wieder mit dem Magier teilen.";
	const char* const text0 = "Sprich mit deinem Assistenten: Oh Gott es tut mir so leid! Das war alles meine Schuld! Ich muss was übersehen haben! Aber ich wüsste nicht was... Ich hatte alles kontrolliert! Ich... ich schwör‘s! Nach mir hat sogar der Dompteur noch die Seile gecheckt!";
	const char* const text1 = "Sprich mit deinem Assistenten: Ich hab mich für dich ein bisschen Umgehört... Gerüchten zufolge hatten der Magier und sein Assistent einen großen Streit über die neuen Zirkusplakate. Eventuell solltest du mal mit des Assistenten Reden.";
	const char* const text2 = "Sprich mit dem Dompteur: Ich war nicht an deinen Seilen dran. Warum sollte ich auch? Ich hab nach dem Lieblingsspielzeug von Schnuffel gesucht. Der arme Löwe kann ohne das Ding nicht schlafen. Einer der Clowns war auch da und hat mir beim Suchen geholfen.";
	const char* const text3 = "Sprich mit dem Dompteur: Ich hab dir doch gesagt ich weiß von nichts! Sprich mit dem Magier. Ich hab mir sagen lassen er war ziemlich wütend darüber, nicht mehr die Nummer 1 zu sein...";
	const char* const text4 = "Sprich mit dem Dompteur: Ich wette wer auch immer dein Seil durchgeschnitten hat, hat meinem Baby auch das Spielzeug geklaut... hat du schon mit dem Magier geredet?";
	const char* const text5 = "Sprich mit Clown 1: Hast du Schnuffels Spielzeug gesehen? Wir können es nirgends finden... Hast du diesen komischen Typen gesehen der hier vor dem Zelt rumlungert? Ich wette der hat es geklaut!";
	const char* const text6 = "Sprich mit Clown 1: Hast du mal gesehen wie der Assistent des Magiers dich anstarrt? Der ist neidisch sag ich dir... Das ist kein hübscher Charakterzug von ihm.";
	const char* const text7 = "Sprich mit Clown 1: Der Assistentin des Magiers war's! Ich hab es nicht gesehen aber ich weiß es! Nenn es clownische Intuition!";
	const char* const text8 = "Sprich mit Clown 2: Zwei seltsame Clowns sind seltsamer als ein nicht seltsamer Clown! Das ist doch logisch... Clowns setzen auf Masse statt Klasse... Wo wir gerade bei seltsam sind, hast du diesen Typen hier rumschleichen sehen?";
	const char* const text9 = "Sprich mit Clown2: Seltsame Dinge passieren hier. Seltsam seltsam, so seltsam wie Clowns. Oder der komische Typ, der hier herumschleicht.";
	const char* const text10 = "Sprich mit Clown 3: Ich habe gesehen wer´s war! Zumindest glaube ich das... nein... ich bin mir sicher! Es war der Clown! Ich hab ihn an seinem Gang erkannt! Oder war es doch der Assistent des Magiers? Ach, die beiden sehen sich viel zu ähnlich...";
	const char* const text11 = "Sprich mit Clown 4: Natürlich verhalten wir uns seltsam. Wir sind Clowns, das liegt in unserer Natur! Wäre ein nicht seltsamer Clown nicht viel seltsamer als ein seltsamer Clown? Bestimmt... aber wäre er auch Seltsamer als 2 seltsame Clown? Sprich mal mit meinem Kollegen der kennt die Antwort.";
	const char* const text12 = "Sprich mit Clown 4: Manche Leute finden Clowns gruselig. Auch Manche Clowns finden Clowns gruselig... stell dir vor du würdest dich jedes Mal erschrecken, wenn du in den Spiegel siehst!";
	const char* const text13 = "Sprich mit dem strangen Dude: Ja, ich habe deinen Unfall gesehen. Schrecklich! Aber ich habe damit nichts zu tun! Ich bin wegen des Löwens hier! Er ist so niedlich! Und flauschig! Und wunderschön! So eine große Miezekatze! Ich habe aber gestern einen der Clowns hier rumschleichen sehen... es geht mich ja nichts an aber der sah mir schon verdächtig aus...";
	const char* const text14 = "Sprich mit dem strangen Dude: Ich durfte die Miezekatze streicheln! Er ist so niedlich! Aber er hat sein Spielzeug verloren... armes Tierchen... einer der Clowns wollte suchen helfen.";
	const char* const text15 = "Sprich mit dem strangen Dude: Ich habe mit einem Clown geredet... frag mich nicht mit welchem... aber er weiß wer es war!";
	const char* const text16 = "Sprich mit dem Magier: Ich? Neidisch auf deinen Erfolg? Nein, niemals! Ich werde alt, meine Zeit hier geht zu Ende. Ich freue mich zu sehen, dass die nächste Generation dem Zirkus gut tut! Wo wir grade reden: Weißt du was mit den Clowns los ist? Die verhalten sich alle seltsam...";
	const char* const text17 = "Sprich mit dem Magier: Eventuell kann dir dein Assistent ja bei der Suche weiter helfen. Wozu sind Assistenten denn sonst da?";
	const char* const text18 = "Sprich mit dem Assistenten des Magiers: Du glaubst also nicht dass dein Unfall ein Unfall war? Ha! Dann seid ihr Artisten doch nicht so dumm wie ich dachte! Geschieht dir trotzdem Recht! Einfach meinem Boss die Show stehlen! Jeder kann turnen wenn er nur ein bisschen übt aber Magie? Das ist noch eine echte Kunstform! Du hast es verdient und ich würde es wieder tun!";
	const char* const text19 = "Sprich mit dem Zirkusdirektor: Nicht zu fassen, dass so etwas in meinem Zirkus passiert! Ich glaube nicht daran, dass es kein Unfall war. Niemand aus meinem Zirkus würde so etwas tun! Am wenigsten der Magier, auch wenn manche behaupten er beneide dich um deinen Erfolg... Geh doch zu ihm und überzeug dich selbst davon";
	
	Sound* intro;
	Sound* outro;
	Sound* assistent_1;
	Sound* assistent_2;
	Sound* dompteur_1;
	Sound* dompteur_2;
	Sound* dompteur_3;
	Sound* clown1_1;
	Sound* clown1_2;
	Sound* clown1_3;
	Sound* clown2_1;
	Sound* clown2_2;
	Sound* clown3_1;
	Sound* clown4_1;
	Sound* clown4_2;
	Sound* dude_1;
	Sound* dude_2;
	Sound* dude_3;
	Sound* magier;
	Sound* assistent_magier;
	Sound* zirkusdirektor;
	
	intro = new Sound("sound/story/intro.wav");
	outro = new Sound("sound/story/outro.wav");
	assistent_1 = new Sound("sound/story/assistent_1.wav");
	assistent_2 = new Sound("sound/story/assistent_2.wav");
	dompteur_1 = new Sound("sound/story/dompteur_1.wav");
	dompteur_2 = new Sound("sound/story/dompteur_2.wav");
	dompteur_3 = new Sound("sound/story/dompteur_3.wav");
	clown1_1 = new Sound("sound/story/clown1_1.wav");
	clown1_2 = new Sound("sound/story/clown1_2.wav");
	clown1_3 = new Sound("sound/story/clown1_3.wav");
	clown2_1 = new Sound("sound/story/clown2_1.wav");
	clown2_2 = new Sound("sound/story/clown2_2.wav");
	clown3_1 = new Sound("sound/story/clown3_1.wav");
	clown4_1 = new Sound("sound/story/clown4_1.wav");
	clown4_2 = new Sound("sound/story/clown4_2.wav");
	dude_1 = new Sound("sound/story/dude_1.wav");
	dude_2 = new Sound("sound/story/dude_2.wav");
	dude_3 = new Sound("sound/story/dude_3.wav");
	magier = new Sound("sound/story/magier.wav");;
	assistent_magier = new Sound("sound/story/assistent_magier.wav");
	zirkusdirektor = new Sound("sound/story/zirkusdirektor.wav");
	
	currentID = 0;
	createNewNode(0, None, introText, intro);
	createNewNode(1, Assistent, text0, assistent_1);
	createNewNode(2, Zirkusdirektor, text19, zirkusdirektor);
	createNewNode(3, Dompteur, text2, dompteur_1);
	createNewNode(4, Zirkusdirektor, text19, zirkusdirektor);
	createNewNode(5, Magier, text16, magier);
	createNewNode(6, Dompteur, text2, dompteur_1);
	createNewNode(7, Clown1, text5, clown1_1);
	createNewNode(9, Magier, text16, magier);
	createNewNode(10, Assistent, text1, assistent_2);
	createNewNode(11, Clown4, text12, clown4_2);
	createNewNode(12, Dompteur, text2, dompteur_1);
	createNewNode(13, Dude, text14, dude_2);
	createNewNode(14, Assistent, text1, assistent_2);
	createNewNode(15, Dude, text13, dude_1);
	createNewNode(19, Clown4, text11, clown4_1);
	createNewNode(20, Dompteur, text2, dompteur_1);
	createNewNode(21, AssistentMagier, text18, assistent_magier);
	createNewNode(23, Clown2, text8, clown2_1);
	createNewNode(24, Dompteur, text2, dompteur_1);
	createNewNode(25, Dude, text14, dude_2);
	createNewNode(26, Assistent, text1, assistent_2);
	createNewNode(27, Clown3, text10, clown3_1);
	createNewNode(29, AssistentMagier, text18, assistent_magier);
	createNewNode(31, Dompteur, text3, dompteur_2);
	createNewNode(32, Clown2, text9, clown2_2);
	createNewNode(39, Clown2, text8, clown2_1);
	createNewNode(40, Clown1, text7, clown1_3);
	createNewNode(41, Assistent, text1, assistent_2);
	createNewNode(42, Magier, text17, magier);
	createNewNode(47, Dude, text14, dude_2);
	createNewNode(48, Assistent, text1, assistent_2);
	createNewNode(49, Clown4, text12, clown4_2);
	createNewNode(50, Assistent, text1, assistent_2);
	createNewNode(51, Clown3, text10, clown3_1);
	createNewNode(53, AssistentMagier, text18, assistent_magier);
	createNewNode(55, AssistentMagier, text18, assistent_magier);
	createNewNode(63, Magier, text16, magier);
	createNewNode(64, Dude, text15, dude_3);
	createNewNode(65, Dude, text15, dude_3);
	createNewNode(66, Dompteur, text4, dompteur_3);
	createNewNode(79, Dude, text14, dude_2);
	createNewNode(81, AssistentMagier, text18, assistent_magier);
	createNewNode(83, AssistentMagier, text18, assistent_magier);
	createNewNode(85, AssistentMagier, text18, assistent_magier);
	createNewNode(95, Clown3, text10, clown3_1);
	createNewNode(96, Clown1, text7, clown1_3);
	createNewNode(97, AssistentMagier, text18, assistent_magier);
	createNewNode(99, Clown2, text9, clown2_2);
	createNewNode(100, Assistent, text1, assistent_2);
	createNewNode(101, AssistentMagier, text18, assistent_magier);
	createNewNode(103, AssistentMagier, text18, assistent_magier);
	createNewNode(127, Clown4, text11, clown4_1);
	createNewNode(128, Clown1, text7, clown1_3);
	createNewNode(129, Clown3, text10, clown3_1);
	createNewNode(130, Clown1, text7, clown1_3);
	createNewNode(131, Clown3, text10, clown3_1);
	createNewNode(132, Clown1, text7, clown1_3);
	createNewNode(133, Clown1, text7, clown1_3);
	createNewNode(134, Assistent, text1, assistent_2);
	createNewNode(159, Clown3, text10, clown3_1);
	createNewNode(160, Dompteur, text3, dompteur_2);
	createNewNode(191, AssistentMagier, text18, assistent_magier);
	createNewNode(193, AssistentMagier, text18, assistent_magier);
	createNewNode(199, Dude, text14, dude_2);
	createNewNode(200, Clown1, text7, clown1_3);
	createNewNode(201, AssistentMagier, text18, assistent_magier);
	createNewNode(255, Clown2, text8, clown2_1);
	createNewNode(256, Assistent, text1, assistent_2);
	createNewNode(257, AssistentMagier, text18, assistent_magier);
	createNewNode(259, AssistentMagier, text18, assistent_magier);
	createNewNode(261, AssistentMagier, text18, assistent_magier);
	createNewNode(263, AssistentMagier, text18, assistent_magier);
	createNewNode(265, AssistentMagier, text18, assistent_magier);
	createNewNode(267, AssistentMagier, text18, assistent_magier);
	createNewNode(269, AssistentMagier, text18, assistent_magier);
	createNewNode(319, AssistentMagier, text18, assistent_magier);
	createNewNode(321, Clown4, text12, clown4_2);
	createNewNode(322, Clown1, text7, clown1_3);
	createNewNode(399, Clown3, text10, clown3_1);
	createNewNode(401, AssistentMagier, text18, assistent_magier);
	createNewNode(511, Dude, text14, dude_2);
	createNewNode(512, Clown1, text5, clown1_2);
	createNewNode(513, AssistentMagier, text18, assistent_magier);
	createNewNode(643, Clown2, text9, clown2_2);
	createNewNode(645, AssistentMagier, text18, assistent_magier);
	createNewNode(644, Assistent, text1, assistent_2);
	createNewNode(799, AssistentMagier, text18, assistent_magier);
	createNewNode(1023, Clown3, text10, clown3_1);
	createNewNode(1024, Clown1, text5, clown1_2);
	createNewNode(1025, AssistentMagier, text18, assistent_magier);
	createNewNode(1287, Dude, text15, dude_3);
	createNewNode(1288, Clown1, text7, clown1_3);
	createNewNode(1289, AssistentMagier, text18, assistent_magier);
	createNewNode(2047, AssistentMagier, text18, assistent_magier);
	createNewNode(2049, AssistentMagier, text18, assistent_magier);
	createNewNode(2575, Clown3, text10, clown3_1);
	createNewNode(2577, AssistentMagier, text18, assistent_magier);
	createNewNode(5151, AssistentMagier, text18, assistent_magier);
	createNewNode(5152, None, outroText, outro);
}

BinaryTree::~BinaryTree() {
	delete []nodes;
}

void BinaryTree::createNewNode(int index, Character speakWithCharacter, const char* const data, Sound* audio) {
	nodes[index] = new Node(currentID, speakWithCharacter, data, audio);
}

Node* BinaryTree::getCurrentNode() const {
	return nodes[currentID];
}

Node* BinaryTree::getLastNode() const {
	return nodes[numOfNodes];
}

int BinaryTree::getCurrentNodeID() const {
	return currentID;
}

Node* BinaryTree::getLeftNode() const {
	return nodes[2 * currentID + 1];
}

Node* BinaryTree::getRightNode() const {
	return nodes[2 * currentID + 2];
}

Sound* BinaryTree::getAudio() const {
	return nodes[currentID]->getAudio();
}

bool BinaryTree::setCurrentNode(int ID) {
	if (ID < numOfNodes) {
		currentID = ID;
		return true;
	}
	return false;
}
