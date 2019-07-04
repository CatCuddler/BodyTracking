#include "pch.h"
#include "BinaryTree.h"

#include <Kore/Log.h>

using namespace Kore;

BinaryTree::BinaryTree() {
	
	const char* const text0 = "Deine Karriere läuft gut. Du arbeitest im Zirkus - ein Traum, den du schon von Kindheit an verfolgst. Vor Kurzen kam es noch besser: Der Zirkusdirektor erzählt di, du seist die Hauptattraktion! Dein Gesicht ist auf allen Plakaten zu sehen! Du bist ein Star und reist mit dem Zirkus um die ganze Welt. Bis es zu einem Fürchterlichen Unfall kommt. Bei einer Abendvorstellung reißt ein Seil deines Trapezes und du stürzt. Wie durch ein Wunder kommst du ohne größere Verletzungen davon. Aber deine Karriere ist erst einmal auf Eis gelegt.Du glaubst nicht daran, dass es ein Unfall war. Dein Assistent hatte die Seile Extra noch kontrolliert! Du machst dich auf die Suche nach dem Täter.";
	const char* const text1 = "Sprich mit deinem Assistenten: Oh Gott es tut mir so leid! Das war alles meine Schuld! Ich muss was übersehen haben! Aber ich wüsste nicht was... Ich hatte alles kontrolliert! Ich... ich schwör‘s! Nach mir hat sogar der Dompteur noch die Seile Gecheckt!";
	const char* const text2 = "Sprich mit dem Dompteur: Ich war nicht an deinen Seilen dran. Warum sollte ich auch? Ich hab nach dem Lieblingsspielzeug von Schnuffel gesucht. Der arme Löwe kann ohne das Ding nicht schlafen. Einer der Clowns war auch da und hat mir beim Suchen geholfen.";
	const char* const text3 = "Sprich mit Clown 1: Hast du Schnuffels Spielzeug gesehen? Wir können es nirgends finden... Hast du diesen komischen Typen gesehen der hier vor dem Zelt rumlungert? Ich wette der hat es geklaut!";
	const char* const text4 = "Sprich mit dem strangen Dude: Ja, ich habe deinen Unfall gesehen. Schrecklich! Aber ich habe damit nichts zu tun! Ich bin wegen des Löwens hier! Er ist so niedlich! Und flauschig! Und wunderschön! So eine große Miezekatze! Ich habe aber gestern einen der Clowns hier rumschleichen sehen... es geht mich ja nichts an aber der sah mir schon verdächtig aus...";
	const char* const text5 = "Sprich mit dem Dompteur: Ich hab dir doch gesagt ich weiß von nichts! Sprich mit dem Magier. Ich hab mir sagen lassen er war ziemlich wütend darüber, nicht mehr die Nummer 1 zu sein...";
	const char* const text6 = "Sprich mit dem Magier: Ich? Neidisch auf deinen Erfolg? Nein, niemals! Ich werde alt, meine Zeit hier geht zu Ende. Ich freue mich zu sehen, dass die nächste Generation dem Zirkus gut tut! Wo wir grade reden: Weißt du was mit den Clowns los ist? Die verhalten sich alle seltsam...";
	const char* const text7 = "Sprich mit Clown 4: Natürlich verhalten wir uns seltsam. Wir sind Clowns, das liegt in unserer Natur! Wäre ein nicht seltsamer Clown nicht viel seltsamer als ein seltsamer Clown? Bestimmt... aber wäre er auch Seltsamer als 2 seltsame Clown? Sprich mal mit meinem Kollegen der kennt die Antwort.";
	const char* const text8 = "Sprich mit Clown 2: Zwei seltsame Clowns sind seltsamer als ein nicht seltsamer Clown! Das ist doch logisch... Clowns setzen auf Masse statt Klasse... Wo wir gerade bei seltsam sind, hast du diesen Typen hier rumschleichen sehen?";
	const char* const text9 = "Sprich mit dem strangen Dude: Ich durfte die Miezekatze streicheln! Er ist so niedlich! Aber er hat sein Spielzeug verloren... armes Tierchen... einer der Clowns wollte suchen helfen.";
	const char* const text10 = "Sprich mit Clown 3: Ich habe gesehen wer´s war! Zumindest glaube ich das... nein... ich bin mir sicher! Es war der Clown! Ich hab ihn an seinem Gang erkannt! Oder war es doch der Assistent des Magiers? Ach, die beiden sehen sich viel zu ähnlich...";
	const char* const text11 = "Sprich mit dem Assistenten des Magiers: Du glaubst also nicht dass dein Unfall ein Unfall war? Ha! Dann seid ihr Artisten doch nicht so dumm wie ich dachte! Geschieht dir trotzdem Recht! Einfach meinem Boss die Show stehlen! Jeder kann turnen wenn er nur ein bisschen übt aber Magie? Das ist noch eine echte Kunstform! Du hast es verdient und ich würde es wieder tun!";
	const char* const text12 = "Sprich mit Clown 1: Hast du mal gesehen wie der Assistent des Magiers dich anstarrt? Der ist neidisch sag ich dir... Das ist kein hübscher Charakterzug von ihm.";
	const char* const text16 = "Sprich mit deinem Assistenten: Ich hab mich für dich ein bisschen Umgehört... Gerüchten zufolge hatten der Magier und sein Assistent einen großen Streit über die neuen Zirkusplakate. Eventuell solltest du mal mit des Assistenten Reden.";
	const char* const text18 = "Sprich mit Clown 1: Der Assistenz des Magiers war's! Ich hab es nicht gesehen aber ich weiß es! Nenn es clownische Intuition!";
	const char* const text25 = "Sprich mit Clown2: Seltsame Dinge passieren hier. Seltsam seltsam, so seltsam wie Clowns. Oder der komische Typ, der hier herumschleicht.";
	const char* const text26 = "Sprich mit dem strangen Dude: Ich habe mit einem Clown geredet... frag mich nicht mit welchem... aber er weiß wer es war!";
	const char* const text31 = "Sprich mit dem Dompteur: Ich wette wer auch immer dein Seil durchgeschnitten hat, hat meinem Baby auch das Spielzeug geklaut... hat du schon mit dem Magier geredet?";
	const char* const text36 = "Sprich mit dem Zirkusdirektor: Nicht zu fassen, dass so etwas in meinem Zirkus passiert! Ich glaube nicht daran, dass es kein Unfall war. Niemand aus meinem Zirkus würde so etwas tun! Am wenigsten der Magier, auch wenn manche behaupten er beneide dich um deinen Erfolg... Geh doch zu ihm und überzeug dich selbst davon";
	const char* const text60 = "Sprich mit dem Magier: Eventuell kann dir dein Assistent ja bei der Suche weiter helfen. Wozu sind Assistenten denn sonst da?";
	
	Node* root = new Node(None, text0);
	Node* node1 = new Node(Assistent, text1);
	Node* node2 = new Node(Dompteur, text2);
	Node* node3 = new Node(Clown1, text3);
	Node* node4 = new Node(Dude, text4);
	Node* node5 = new Node(Dompteur, text5);
	Node* node6 = new Node(Magier, text6);
	Node* node7 = new Node(Clown4, text7);
	Node* node8 = new Node(Zirkusdirektor, text8);
	Node* node9 = new Node(Zirkusdirektor, text9);
	Node* node10 = new Node(Clown3, text10);
	Node* node11 = new Node(AssistentMagier, text11);
	Node* node12 = new Node(Clown1, text12);
	Node* node13 = new Node(AssistentMagier, text11);
	Node* node14 = new Node(Clown1, text12);
	Node* node15 = new Node(AssistentMagier, text11);
	Node* node16 = new Node(Assistent, text16);
	Node* node17 = new Node(AssistentMagier, text11);
	Node* node18 = new Node(Clown1, text18);
	Node* node19 = new Node(AssistentMagier, text11);
	Node* node20 = new Node(Dude, text4);
	Node* node21 = new Node(Clown3, text10);
	Node* node22 = new Node(AssistentMagier, text11);
	Node* node23 = new Node(Clown1, text18);
	Node* node24 = new Node(AssistentMagier, text11);
	Node* node25 = new Node(Clown2, text25);
	Node* node26 = new Node(AssistentMagier, text26);
	Node* node27 = new Node(Clown3, text10);
	Node* node28 = new Node(AssistentMagier, text11);
	Node* node29 = new Node(Clown1, text18);
	Node* node30 = new Node(AssistentMagier, text11);
	Node* node31 = new Node(Dompteur, text31);
	Node* node32 = new Node(Clown1, text18);
	Node* node33 = new Node(AssistentMagier, text11);
	Node* node34 = new Node(Assistent, text16);
	Node* node35 = new Node(AssistentMagier, text11);
	Node* node36 = new Node(Zirkusdirektor, text36);
	Node* node37 = new Node(Magier, text6);
	Node* node38 = new Node(Clown4, text7);
	Node* node39 = new Node(Clown2, text8);
	Node* node40 = new Node(Dude, text9);
	Node* node41 = new Node(Clown3, text10);
	Node* node42 = new Node(AssistentMagier, text11);
	Node* node43 = new Node(Dompteur, text5);
	Node* node44 = new Node(Clown4, text7);
	Node* node45 = new Node(Clown2, text8);
	Node* node46 = new Node(Dude, text9);
	Node* node47 = new Node(Clown3, text10);
	Node* node48 = new Node(AssistentMagier, text11);
	Node* node49 = new Node(Clown1, text18);
	Node* node50 = new Node(AssistentMagier, text11);
	Node* node51 = new Node(Assistent, text16);
	Node* node52 = new Node(AssistentMagier, text11);
	Node* node53 = new Node(Clown1, text18);
	Node* node54 = new Node(AssistentMagier, text11);
	Node* node55 = new Node(Clown1, text18);
	Node* node56 = new Node(AssistentMagier, text11);
	Node* node57 = new Node(Dompteur, text2);
	Node* node58 = new Node(Assistent, text16);
	Node* node59 = new Node(AssistentMagier, text11);
	Node* node60 = new Node(Magier, text60);
	Node* node61 = new Node(AssistentMagier, text11);
	Node* node62 = new Node(Assistent, text16);
	Node* node63 = new Node(AssistentMagier, text11);
	Node* node64 = new Node(Magier, text6);
	Node* node65 = new Node(Clown4, text7);
	Node* node66 = new Node(Clown2, text8);
	Node* node67 = new Node(Dude, text9);
	Node* node68 = new Node(Clown3, text10);
	Node* node69 = new Node(AssistentMagier, text11);
	Node* node70 = new Node(Clown1, text18);
	Node* node71 = new Node(AssistentMagier, text11);
	Node* node72 = new Node(Assistent, text16);
	Node* node73 = new Node(AssistentMagier, text11);
	Node* node74 = new Node(Dompteur, text2);
	Node* node75 = new Node(Clown4, text7);
	Node* node76 = new Node(Clown2, text8);
	Node* node77 = new Node(Dude, text9);
	Node* node78 = new Node(Clown3, text10);
	Node* node79 = new Node(AssistentMagier, text11);
	Node* node80 = new Node(Clown1, text18);
	Node* node81 = new Node(AssistentMagier, text11);
	
	// Test
	//const char* const textX = "(x) Sprich mit dem Zirkusdirektor";
	//Node* nodeX = new Node(AssistentMagier, textX);
	
	
	root->setLeftNode(node1);
	root->setRightNode(node63);
	node1->setLeftNode(node2);
	node1->setRightNode(node36);
	node2->setLeftNode(node3);
	node2->setRightNode(nullptr);
	node3->setLeftNode(node4);
	node3->setRightNode(nullptr);
	node4->setLeftNode(node5);
	node4->setRightNode(node25);
	node5->setLeftNode(node6);
	node5->setRightNode(node20);
	node6->setLeftNode(node7);
	node6->setRightNode(node18);
	node7->setLeftNode(node8);
	node7->setRightNode(node16);
	node8->setLeftNode(node9);
	node8->setRightNode(node14);
	node9->setLeftNode(node10);
	node9->setRightNode(node12);
	node10->setLeftNode(node11);
	node10->setRightNode(nullptr);
	node11->setLeftNode(nullptr);
	node11->setRightNode(nullptr);
	node12->setLeftNode(node13);
	node12->setRightNode(nullptr);
	node13->setLeftNode(nullptr);
	node13->setRightNode(nullptr);
	node14->setLeftNode(node15);
	node14->setRightNode(nullptr);
	node15->setLeftNode(nullptr);
	node15->setRightNode(nullptr);
	node16->setLeftNode(node17);
	node16->setRightNode(nullptr);
	
	//nodeX->setLeftNode(nullptr);
	//nodeX->setRightNode(nullptr);
	
	currentNode = root;
}

Node* BinaryTree::getCurrentNode() {
	return currentNode;
}

void BinaryTree::getLeftNode() {
	currentNode = currentNode->getLeft();
}

void BinaryTree::getRightNode() {
	currentNode = currentNode->getRight();
}
