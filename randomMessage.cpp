// my_class.cpp
#include "randomMessage.h" // header in local directory
#include <iostream> // header in standard library
#include <vector>
#include <cstdlib>



/*
using namespace N;
using namespace std;
*/

std::vector<std::string> messageVector;


std::string randomMessage::getRandomMessage()
{
    return messageVector.at((rand() % messageVector.size()));
} 


void randomMessage::initializeVector()
{
    messageVector.push_back("There is a theory which states that if ever anyone discovers exactly what the Universe is for and why it is here, it will instantly disappear and be replaced by something even more bizarre and inexplicable. There is another theory which states that this has already happened.");
    messageVector.push_back("Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.");
    messageVector.push_back("My doctor says that I have a malformed public-duty gland and a natural deficiency in moral fibre,” Ford muttered to himself, \"and that I am therefore excused from saving Universes.\"");
    messageVector.push_back("The ships hung in the sky in much the same way that bricks don’t.");
    messageVector.push_back("\"You know,\" said Arthur, \"it’s at times like this, when I’m trapped in a Vogon airlock with a man from Betelgeuse, and about to die of asphyxiation in deep space that I really wish I’d listened to what my mother told me when I was young.\" \"Why, what did she tell you?\"\"I don’t know, I didn’t listen.\"");
    messageVector.push_back("\"Space,\" it says, \"is big. Really big. You just won’t believe how vastly, hugely, mindbogglingly big it is. I mean, you may think it’s a long way down the road to the chemist’s, but that’s just peanuts to space.\"");
    messageVector.push_back("\"Funny,\" he intoned funereally, \"how just when you think life can’t possibly get any worse it suddenly does.\"");
    messageVector.push_back("\"Don't Panic.\"");
    messageVector.push_back("Isn’t it enough to see that a garden is beautiful without having to believe that there are fairies at the bottom of it too?");
    messageVector.push_back("A common mistake that people make when trying to design something completely foolproof is to underestimate the ingenuity of complete fools.");
    messageVector.push_back("The reason why it was published in the form of a micro sub meson electronic component is that if it were printed in normal book form, an interstellar hitchhiker would require several inconveniently large buildings to carry it around in.");
    messageVector.push_back("For instance, on the planet Earth, man had always assumed that he was more intelligent than dolphins because he had achieved so much — the wheel, New York, wars and so on — whilst all the dolphins had ever done was muck about in the water having a good time. But conversely, the dolphins had always believed that they were far more intelligent than man — for precisely the same reasons.");
    messageVector.push_back("The last ever dolphin message was misinterpreted as a surprisingly sophisticated attempt to do a double-backwards-somersault through a hoop whilst whistling the ‘Star Spangled Banner’, but in fact the message was this: So long and thanks for all the fish.");
    messageVector.push_back("The chances of finding out what’s really going on in the universe are so remote, the only thing to do is hang the sense of it and keep yourself occupied.");
    messageVector.push_back("\"Listen, three eyes,\" he said, \"don’t you try to outweird me, I get stranger things than you free with my breakfast cereal.\"");
    messageVector.push_back("\"Forty-two,\" said Deep Thought, with infinite majesty and calm.");
    messageVector.push_back("Not unnaturally, many elevators imbued with intelligence and precognition became terribly frustrated with the mindless business of going up and down, up and down, experimented briefly with the notion of going sideways, as a sort of existential protest, demanded participation in the decision-making process and finally took to squatting in basements sulking.");
    messageVector.push_back("The Total Perspective Vortex derives its picture of the whole Universe on the principle of extrapolated matter analyses.To explain — since every piece of matter in the Universe is in some way affected by every other piece of matter in the Universe, it is in theory possible to extrapolate the whole of creation — every sun, every planet, their orbits, their composition and their economic and social history from, say, one small piece of fairy cake. The man who invented the Total Perspective Vortex did so basically in order to annoy his wife.");
    messageVector.push_back("\"Shee, you guys are so unhip it’s a wonder your bums don’t fall off.\"");
    messageVector.push_back("It is known that there are an infinite number of worlds, simply because there is an infinite amount of space for them to be in. However, not every one of them is inhabited. Therefore, there must be a finite number of inhabited worlds. Any finite number divided by infinity is as near to nothing as makes no odds, so the average population of all the planets in the Universe can be said to be zero. From this it follows that the population of the whole Universe is also zero, and that any people you may meet from time to time are merely the products of a deranged imagination.");
    messageVector.push_back("The disadvantages involved in pulling lots of black sticky slime from out of the ground where it had been safely hidden out of harm’s way, turning it into tar to cover the land with, smoke to fill the air with and pouring the rest into the sea, all seemed to outweigh the advantages of being able to get more quickly from one place to another.");
    messageVector.push_back("Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them. If that means sticking all sort of spikes and prongs and blackened bits all over it then so be it. This is not a gun for hanging over the fireplace or sticking in the umbrella stand, it is a gun for going out and making people miserable with.");
    messageVector.push_back("It is a well known fact that those people who most want to rule people are, ipso facto, those least suited to do it. To summarize the summary: anyone who is capable of getting themselves made President should on no account be allowed to do the job.");
}