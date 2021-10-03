/*
  Super Monkey Call
  Copyright (C) 2021 Bernhard Schelling

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <ZL_Application.h>
#include <ZL_Display.h>
#include <ZL_Surface.h>
#include <ZL_Signal.h>
#include <ZL_Audio.h>
#include <ZL_Font.h>
#include <ZL_Input.h>
#include <ZL_SynthImc.h>
#include <../Opt/chipmunk/chipmunk.h>

extern ZL_SynthImcTrack imcMusic;
extern TImcSongData imcDataIMCGRAB, imcDataIMCTHROW, imcDataIMCGAMEOVER;
static ZL_Sound sndGrab, sndThrow, sndGameOver;
static ZL_Font fntMain;
static cpSpace *space;
static cpBody *bodyTree;
static ZL_Surface srfHill, srfTree, srfMonkey, srfLogo;
static int monkeys;
static ZL_TextBuffer txtMonkeys;
static ZL_Color sky[4];

enum CollisionTypes
{
	COLLISION_NONE,
	COLLISION_TREE,
	COLLISION_MONKEY,
};

static void PostStepAddJoint(cpSpace *space, cpConstraint* joint, void* data)
{
	cpConstraintSetErrorBias(joint, cpfpow(1.0f - 0.001f, 60.0f));
	cpSpaceAddConstraint(space, joint);
}

static void PostStepRemoveBody(cpSpace *space, cpBody* body, void* data)
{
	CP_BODY_FOREACH_SHAPE(body, shape) cpSpaceRemoveShape(space, shape);
	CP_BODY_FOREACH_CONSTRAINT(body, constraint) cpSpaceRemoveConstraint(space, constraint);
	cpSpaceRemoveBody(space, body);
}

static cpBool CollisionMonkey(cpArbiter *arb, cpSpace *space, cpDataPointer userData)
{
	CP_ARBITER_GET_BODIES(arb, bMonkey, bTree);
	cpBodySetVelocity(bMonkey, cpvzero);

	if (bMonkey->constraintList && bTree->constraintList) return cpTrue;
	if (!bMonkey->constraintList && !bTree->constraintList) return cpTrue;

	cpVect hit = (arb->swapped ? cpArbiterGetPointA(arb, 0) : cpArbiterGetPointB(arb, 0));
	cpVect norm = cpArbiterGetNormal(arb); // takes swapped into account
	float dist = cpArbiterGetDepth(arb, 0);
	cpBodySetPosition(bMonkey, cpvadd(bMonkey->p, cpvmult(norm, dist-1)));
	cpVect off = cpv(0, 3);
	cpSpaceAddPostStepCallback(space, (cpPostStepFunc)PostStepAddJoint, cpPinJointNew(bMonkey, bTree,         off, cpBodyWorldToLocal(bTree, cpvadd(hit, off))), NULL);
	cpSpaceAddPostStepCallback(space, (cpPostStepFunc)PostStepAddJoint, cpPinJointNew(bMonkey, bTree, cpvneg(off), cpBodyWorldToLocal(bTree, cpvsub(hit, off))), NULL);

	monkeys++;
	txtMonkeys.SetText(ZL_String::format("%d", monkeys));
	sndGrab.Play();

	return cpTrue;
}

static void Reset()
{
	if (space) cpSpaceFree(space);
	space = cpSpaceNew();
	cpSpaceSetGravity(space, cpv(0.0f, -98.7f));
	cpSpaceAddCollisionHandler(space, COLLISION_MONKEY, COLLISION_TREE)->beginFunc = CollisionMonkey;
	cpSpaceAddCollisionHandler(space, COLLISION_MONKEY, COLLISION_MONKEY)->beginFunc = CollisionMonkey;

	cpBody *b = cpSpaceAddBody(space, cpBodyNewStatic());
	cpBodySetPosition(b, cpv(0, -100));
	cpShape *shape = cpSpaceAddShape(space, cpBoxShapeNew(b, 50, 200, 0));
	cpShapeSetFriction(shape, 1);

	float treemass = 50;
	bodyTree = cpSpaceAddBody(space, cpBodyNew(treemass, cpMomentForCircle(treemass, 0, 50, cpvzero)));
	cpBodySetPosition(bodyTree, cpv(0, 100));
	shape = cpSpaceAddShape(space, cpBoxShapeNew2(bodyTree, cpBBNew( -10, -100,  10,  100), 0)); cpShapeSetCollisionType(shape, COLLISION_TREE);
	shape = cpSpaceAddShape(space, cpBoxShapeNew2(bodyTree, cpBBNew(-70,  100, 70,  130), 0)); cpShapeSetCollisionType(shape, COLLISION_TREE);
	cpConstraint* trunk = cpPivotJointNew(bodyTree, space->staticBody, cpv(0, 1));
	cpConstraintSetMaxForce(trunk, 1000);
	cpSpaceAddConstraint(space, trunk);

	monkeys = 0;
	txtMonkeys.SetText(ZL_String::format("%d", monkeys));

	sky[0] = ZLRGB( RAND_RANGE(.0, .4),  RAND_RANGE(.0, .4), RAND_RANGE(.4, .8) );
	sky[1] = ZLRGB( RAND_RANGE(.0, .4),  RAND_RANGE(.0, .4), RAND_RANGE(.4, .8) );
	sky[2] = ZLRGB( RAND_RANGE(.0, .4),  RAND_RANGE(.0, .4), RAND_RANGE(.4, .8) );
	sky[3] = ZLRGB( RAND_RANGE(.0, .4),  RAND_RANGE(.0, .4), RAND_RANGE(.4, .8) );
}

static void Init()
{
	fntMain = ZL_Font("Data/matchbox.ttf.zip", 52);
	srfHill   = ZL_Surface("Data/hill.png").SetOrigin(ZL_Origin::Center).SetScale(.5f);
	srfTree   = ZL_Surface("Data/tree.png").SetOrigin(ZL_Origin::Center).SetScale(.55f);
	srfMonkey = ZL_Surface("Data/monkey.png").SetOrigin(ZL_Origin::Center).SetScale(.4f);
	srfLogo   = ZL_Surface("Data/logo.png").SetOrigin(ZL_Origin::Center);

	imcMusic.Play();
	sndGrab = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCGRAB);
	sndThrow = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCTHROW);
	sndGameOver = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCGAMEOVER);

	txtMonkeys = ZL_TextBuffer(fntMain);

	Reset();
}

static void DrawMonkey(cpBody *body, void *data)
{
	if (body->shapeList->type != COLLISION_MONKEY) return;

	float size = ((cpCircleShape*)body->shapeList)->r / 12.f * srfMonkey.GetScaleW();
	srfMonkey.Draw(body->p, body->a, size * (cpBodyGetUserData(body) ? -1 : 1), size);

	if (body->p.y < -200.f)
		cpSpaceAddPostStepCallback(space, (cpPostStepFunc)PostStepRemoveBody, body, NULL);
}

static void DrawTextBordered(const ZL_TextBuffer& buf, const ZL_Vector& p, scalar scale = 1, const ZL_Color& colfill = ZLWHITE, const ZL_Color& colborder = ZLBLACK, int border = 2, ZL_Origin::Type origin = ZL_Origin::Center)
{
	for (int i = 0; i < 9; i++) if (i != 4) buf.Draw(p.x+(border*((i%3)-1)), p.y+(border*((i/3)-1)), scale, scale, colborder, origin);
	buf.Draw(p.x, p.y, scale, scale, colfill, origin);
}

static void Draw()
{
	static ticks_t TICKSUM = 0;
	for (TICKSUM += ZLELAPSEDTICKS; TICKSUM > 16; TICKSUM -= 16)
		cpSpaceStep(space, 2*s(16.0/1000.0));

	static ticks_t TICKTITLESTART, TICKTITLEEND, TICKGAMEOVERSTART;
	float title = 0, gameover = 0;
	if (!TICKTITLEEND)
	{
		if (!TICKTITLESTART) TICKTITLESTART = ZLTICKS-1;
		title = ZL_Math::Clamp01(ZLSINCEF(TICKTITLESTART, 1000));
	}
	else if (TICKGAMEOVERSTART)
		gameover = ZL_Math::Clamp01(ZLSINCEF(TICKGAMEOVERSTART, 1000));
	else
		title = 1.0f - ZL_Math::Clamp01(ZLSINCEF(TICKTITLEEND, 1000));

	ZL_Vector camPos = ZLV(0, 100 + ZL_Easing::InCubic(title)*120);
	float camZoom = 200 - ZL_Easing::InCubic(title)*100;

	//float camW, camH;
	//if (ZLWIDTH < ZLHEIGHT) { camW = camZoom; camH = camZoom / ZLASPECTR; }
	//else                    { camW = camZoom * ZLASPECTR; camH = camZoom; }
	float camW = camZoom * ZLASPECTR, camH = camZoom;

	//ZL_Display::FillGradient(0, 0, ZLWIDTH, ZLHEIGHT, ZLRGB(.1,.1,.4), ZLRGB(0,0,.3), ZLRGB(.3,.4,.8), ZLRGB(.4,.4,.8));
	ZL_Display::FillGradient(0, 0, ZLWIDTH, ZLHEIGHT, sky[0], sky[1], sky[2], sky[3]);

	ZL_Display::PushOrtho(camPos.x - camW, camPos.x + camW, camPos.y - camH, camPos.y + camH);

	srfTree.Draw(bodyTree->p, bodyTree->a);
	srfHill.Draw(0, -60);
	cpSpaceEachBody(space, (cpSpaceBodyIteratorFunc)DrawMonkey, NULL);
	ZL_Display::FillGradient(-1000, -100, 1000, 0, ZLLUMA(0,0), ZLLUMA(0,0), ZLLUMA(0,1), ZLLUMA(0,1));

	if (sabs(bodyTree->a) > 1 && space->constraints->num)
	{
		for (int i = space->constraints->num; i--;)
			cpSpaceRemoveConstraint(space, (cpConstraint*)space->constraints->arr[i]);
		sndGameOver.Play();
		TICKGAMEOVERSTART = ZLTICKS;
		gameover = SMALL_NUMBER;
		txtMonkeys.SetText(ZL_String::format("YOU HAD %d MONKEYS ON THE TREE!", monkeys));
	}

	#ifdef ZILLALOG //DEBUG DRAW
	if (ZL_Display::KeyDown[ZLK_LSHIFT])
	{
		ZL_Display::DrawLine(-10000, 0, 10000, 0, ZL_Color::Gray);
		ZL_Display::DrawLine(0, -10000, 0, 10000, ZL_Color::Gray);
		void DebugDrawShape(cpShape*,void*); cpSpaceEachShape(space, DebugDrawShape, NULL);
		void DebugDrawConstraint(cpConstraint*, void*); cpSpaceEachConstraint(space, DebugDrawConstraint, NULL);
	}
	#endif

	if (!title && !gameover)
	{
		ZL_Vector mousepos = ZL_Display::ScreenToWorld(ZL_Input::Pointer());
		float side = ZL_Math::Sign(mousepos.x);
		mousepos.y = ZL_Math::Clamp(mousepos.y, 50.f, 250.f);
		mousepos.x = side * 200.f;

		float range = 0;
		static ticks_t downstart;
		if (ZL_Input::Down()) downstart = ZLTICKS;
		if (downstart && ZLSINCE(downstart) > 100)
		{
			range = (1.f-scos(ZLSINCEF(downstart, 500))) * .5f;

			//if (ZL_Input::Held() && (ZL_Application::FrameCount % 6) == 0) { range = 1;
			if (ZL_Input::Up())
			{
				float scale = RAND_RANGE(.8f, 1.25f);
				cpBody *b = cpSpaceAddBody(space, cpBodyNew(3*scale, cpMomentForCircle(3*scale, 0, 5*scale, cpvzero)));
				cpBodySetUserData(b, (cpDataPointer)(side < 0));
				cpBodySetPosition(b, cpv(300*side, mousepos.y));
				cpShape* shape = cpSpaceAddShape(space, cpCircleShapeNew(b, 12*scale, cpvzero));
				cpShapeSetCollisionType(shape, COLLISION_MONKEY);
				cpBodySetVelocity(b, cpv((100.f + range * 200.f) * -side, 0));
				sndThrow.Play();
				downstart = 0;
			}
		}

		ZL_Vector head = mousepos - ZLV(200 * range * side, 0);
		ZL_Display::DrawWideLine(mousepos, head, 5, ZL_Color::Yellow, ZL_Color::Red);
		ZL_Display::DrawTriangle(head - ZLV(0,10), head - ZLV(10*side,0), head - ZLV(0,-10), ZL_Color::Yellow, ZL_Color::Red);
		ZL_Display::DrawCircle(mousepos, 5, ZL_Color::Yellow, ZL_Color::Red);
	}

	ZL_Display::PopOrtho();

	if (gameover)
	{
		float overup = 400-ZL_Easing::OutBounce(gameover)*400;
		static ZL_TextBuffer txtGameOver(fntMain, "GAME OVER");
		static ZL_TextBuffer txtClickToRestart(fntMain, "CLICK TO RETURN TO TITLE");
		DrawTextBordered(txtGameOver, ZLV(ZLHALFW, ZLHALFH+120-overup), 1.1f, ZL_Color::Red, ZLBLACK, 4);
		DrawTextBordered(txtMonkeys, ZLV(ZLHALFW, ZLHALFH-0-overup*.9f), .9f, ZL_Color::Yellow, ZLBLACK, 4);
		DrawTextBordered(txtClickToRestart, ZLV(ZLHALFW, ZLHALFH-100-overup*.8f), .8f, ZLWHITE, ZLBLACK, 4);
		if ((gameover > .8f && ZL_Input::Up()) || ZL_Input::Down(ZLK_ESCAPE))
		{
			TICKGAMEOVERSTART = TICKTITLEEND = 0, TICKTITLESTART = ZLTICKS;
			Reset();
		}
	}
	else if (!title)
	{
		DrawTextBordered(txtMonkeys, ZLV(ZLHALFW, 100));

		static ticks_t TICKESCAPE;
		if (TICKESCAPE && ZLSINCE(TICKESCAPE) < 1000)
		{
			static ZL_TextBuffer txtEscape(fntMain, "PRESS ESC AGAIN TO QUIT");
			DrawTextBordered(txtEscape, ZLV(ZLHALFW, ZLHALFH));
			if (ZL_Input::Down(ZLK_ESCAPE))
				cpBodyApplyImpulseAtWorldPoint(bodyTree, cpv(10000, 0), cpv(0, 200));
		}
		else if (ZL_Input::Down(ZLK_ESCAPE))
			TICKESCAPE = ZLTICKS;
	}
	else
	{
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, title * .3f));

		float logoup = 450-(TICKTITLEEND ? ZL_Easing::InQuad(title) : ZL_Easing::OutBounce(title))*450;

		for (int i = 0; i != 9; i++)  if (i != 4) srfLogo.Draw(ZLHALFW-3+8*(i/3), ZLHALFH+150+3-8*(i%3)+logoup*.9f, ZLLUMA(0, .5));
		srfLogo.Draw(ZLHALFW, ZLHALFH+150+logoup);

		static ZL_TextBuffer txtClickToPlay(fntMain, "CLICK TO START");
		static ZL_TextBuffer txtInfo(fntMain, "HOLD LEFT MOUSE BUTTON TO FLING MONKEYS");
		static ZL_TextBuffer txtFooter(fntMain, "2021 BERNHARD SCHELLING");

		DrawTextBordered(txtClickToPlay, ZLV(ZLHALFW, 200-logoup), 1.0f, ZL_Color::Yellow, ZLBLACK, 4);
		DrawTextBordered(txtInfo, ZLV(ZLHALFW, 120-logoup*.8f), .8f, ZLWHITE, ZLBLACK, 4);
		DrawTextBordered(txtFooter, ZLV(ZLHALFW, 25-logoup*.5f), .4f, ZLWHITE, ZLBLACK, 3);

		float monkeyrot = ZLSECONDS;
		srfMonkey.Draw(ZLFROMW(150)+10+logoup, ZLFROMH(150)-10, monkeyrot, 2, 2, ZLLUMA(0, .5));
		srfMonkey.Draw(       (150)+10-logoup, ZLFROMH(150)-10, -monkeyrot, -2, 2, ZLLUMA(0, .5));
		srfMonkey.Draw(ZLFROMW(150)   +logoup, ZLFROMH(150), monkeyrot, 2, 2);
		srfMonkey.Draw(       (150)   -logoup, ZLFROMH(150), -monkeyrot, -2, 2);

		if (title > .8f && ZL_Input::Up() && !TICKTITLEEND)
			TICKTITLEEND = ZLTICKS;
		if (ZL_Input::Down(ZLK_ESCAPE) && !TICKTITLEEND)
			ZL_Application::Quit();
	}
}

static struct sWobblezilla : public ZL_Application
{
	sWobblezilla() : ZL_Application(60) { }

	virtual void Load(int argc, char *argv[])
	{
		if (!ZL_Application::LoadReleaseDesktopDataBundle()) return;
		if (!ZL_Display::Init("Super Monkey Call", 1280, 720, ZL_DISPLAY_ALLOWRESIZEHORIZONTAL)) return;
		ZL_Display::ClearFill(ZL_Color::White);
		ZL_Display::SetAA(true);
		ZL_Audio::Init();
		ZL_Input::Init();
		Init();
	}

	virtual void AfterFrame()
	{
		Draw();
	}
} Wobblezilla;

#ifdef ZILLALOG //DEBUG DRAW
void DebugDrawShape(cpShape *shape, void*)
{
	switch (shape->klass->type)
	{
		case CP_CIRCLE_SHAPE: {
			cpCircleShape *circle = (cpCircleShape *)shape;
			ZL_Display::DrawCircle(circle->tc, circle->r, ZL_Color::Green);
			break; }
		case CP_SEGMENT_SHAPE: {
			cpSegmentShape *seg = (cpSegmentShape *)shape;
			cpVect vw = cpvclamp(cpvperp(cpvsub(seg->tb, seg->ta)), seg->r);
			//ZL_Display::DrawLine(seg->ta, seg->tb, ZLWHITE);
			ZL_Display::DrawQuad(seg->ta.x + vw.x, seg->ta.y + vw.y, seg->tb.x + vw.x, seg->tb.y + vw.y, seg->tb.x - vw.x, seg->tb.y - vw.y, seg->ta.x - vw.x, seg->ta.y - vw.y, ZLRGBA(0,1,1,.35), ZLRGBA(1,1,0,.35));
			ZL_Display::DrawCircle(seg->ta, seg->r, ZLRGBA(0,1,1,.35), ZLRGBA(1,1,0,.35));
			ZL_Display::DrawCircle(seg->tb, seg->r, ZLRGBA(0,1,1,.35), ZLRGBA(1,1,0,.35));
			break; }
		case CP_POLY_SHAPE: {
			cpPolyShape *poly = (cpPolyShape *)shape;
			{for (int i = 1; i < poly->count; i++) ZL_Display::DrawLine(poly->planes[i-1].v0, poly->planes[i].v0, ZLWHITE);}
			ZL_Display::DrawLine(poly->planes[poly->count-1].v0, poly->planes[0].v0, ZLWHITE);
			break; }
	}
	ZL_Display::FillCircle(cpBodyGetPosition(shape->body), 3, ZL_Color::Red);
	ZL_Display::DrawLine(cpBodyGetPosition(shape->body), (ZL_Vector&)cpBodyGetPosition(shape->body) + ZLV(cpBodyGetAngularVelocity(shape->body)*-10, 0), ZLRGB(1,0,0));
	ZL_Display::DrawLine(cpBodyGetPosition(shape->body), (ZL_Vector&)cpBodyGetPosition(shape->body) + ZL_Vector::FromAngle(cpBodyGetAngle(shape->body))*10, ZLRGB(1,1,0));
}

void DebugDrawConstraint(cpConstraint *constraint, void *data)
{
	cpBody *body_a = constraint->a, *body_b = constraint->b;

	if(cpConstraintIsPinJoint(constraint))
	{
		cpPinJoint *joint = (cpPinJoint *)constraint;
		cpVect a = (cpBodyGetType(body_a) == CP_BODY_TYPE_KINEMATIC ? body_a->p : cpTransformPoint(body_a->transform, joint->anchorA));
		cpVect b = (cpBodyGetType(body_b) == CP_BODY_TYPE_KINEMATIC ? body_b->p : cpTransformPoint(body_b->transform, joint->anchorB));
		ZL_Display::DrawLine(a.x, a.y, b.x, b.y, ZL_Color::Magenta);
	}
	else if (cpConstraintIsPivotJoint(constraint))
	{
		cpPivotJoint *joint = (cpPivotJoint *)constraint;
		cpVect a = (cpBodyGetType(body_a) == CP_BODY_TYPE_KINEMATIC ? body_a->p : cpTransformPoint(body_a->transform, joint->anchorA));
		cpVect b = (cpBodyGetType(body_b) == CP_BODY_TYPE_KINEMATIC ? body_b->p : cpTransformPoint(body_b->transform, joint->anchorB));
		ZL_Display::DrawLine(a.x, a.y, b.x, b.y, ZL_Color::Magenta);
		ZL_Display::FillCircle(a, 2, ZL_Color::Magenta);
		ZL_Display::FillCircle(b, 2, ZL_Color::Magenta);
	}
	else if (cpConstraintIsRotaryLimitJoint(constraint))
	{
		cpRotaryLimitJoint *joint = (cpRotaryLimitJoint *)constraint;
		cpVect a = cpTransformPoint(body_a->transform, cpvzero);
		cpVect b = cpvadd(a, cpvmult(cpvforangle(joint->min), 40));
		cpVect c = cpvadd(a, cpvmult(cpvforangle(joint->max), 40));
		ZL_Display::DrawLine(a.x, a.y, b.x, b.y, ZL_Color::Magenta);
		ZL_Display::DrawLine(a.x, a.y, c.x, c.y, ZL_Color::Magenta);
	}
}
#endif

// SOUND / MUSIC DATA

static const unsigned int IMCMUSIC_OrderTable[] = {
	0x010000020, 0x010000011, 0x010000011, 0x010000012, 0x010001111, 0x010002212, 0x010001111, 0x010002212,
	0x010001113, 0x010002214, 0x010001113, 0x010002213, 0x010001114, 0x010002211, 0x010001112, 0x010002213,
	0x010001114, 0x010002213, 0x010001113, 0x010002214, 0x010001100, 0x010002200, 0x010000000,
};
static const unsigned char IMCMUSIC_PatternData[] = {
	0x54, 0, 0x54, 0, 0x54, 0x57, 0, 0x57, 0, 0x57, 0x54, 0, 0x52, 0, 0x54, 0x54,
	0, 0x52, 0x52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x54, 0, 0x54, 0, 0x50, 0x52, 0, 0, 0x52, 0x52, 0x54, 0x55, 0x52, 0x54, 0, 0,
	0x57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x42, 0, 0, 0, 0x42, 0x44, 0, 0, 0x42, 0, 0, 0, 0x42, 0x42, 0, 0,
	0x50, 0, 0, 0, 0x50, 0, 0, 0, 0x50, 0, 0x52, 0, 0x54, 0, 0, 0,
	0x62, 0x60, 0x5B, 0, 0, 0, 0, 0, 0x57, 0x55, 0x54, 0, 0, 0, 0, 0,
	0x62, 0x60, 0x5B, 0, 0, 0, 0, 0, 0x57, 0x5B, 0x59, 0x59, 0x59, 0, 0, 0,
	0x62, 0x60, 0x5B, 0, 0, 0, 0, 0, 0x57, 0x55, 0x54, 0, 0, 0, 0, 0,
	0x62, 0x60, 0x5B, 0, 0, 0, 0, 0, 0x57, 0x5B, 0x59, 0x59, 0x59, 0, 0, 0,
	0x55, 0, 0x55, 0, 0x55, 0, 0x55, 0, 0x55, 0, 0x55, 0, 0x55, 0, 0x55, 0,
};
static const unsigned char IMCMUSIC_PatternLookupTable[] = { 0, 4, 6, 8, 10, 10, 10, 10, };
static const TImcSongEnvelope IMCMUSIC_EnvList[] = {
	{ 0, 256, 11, 8, 16, 255, true, 255, },
	{ 0, 256, 180, 0, 16, 255, true, 255, },
	{ 0, 256, 152, 0, 16, 255, true, 255, },
	{ 0, 256, 1096, 0, 16, 16, true, 255, },
	{ 0, 256, 1096, 8, 16, 16, true, 255, },
	{ 0, 256, 726, 8, 255, 255, true, 255, },
	{ 0, 256, 1083, 0, 255, 255, true, 255, },
	{ 0, 256, 3077, 24, 96, 96, true, 255, },
	{ 0, 256, 5231, 8, 255, 255, true, 255, },
	{ 128, 256, 1534, 8, 255, 255, true, 255, },
	{ 0, 256, 94, 8, 255, 255, true, 255, },
	{ 0, 256, 15, 8, 16, 255, true, 255, },
	{ 128, 256, 348, 8, 255, 255, true, 255, },
	{ 0, 256, 64, 8, 8, 255, true, 255, },
	{ 0, 256, 15, 8, 16, 255, true, 255, },
	{ 0, 256, 5, 8, 16, 255, false, 255, },
	{ 0, 256, 379, 8, 16, 255, true, 255, },
	{ 32, 256, 196, 8, 16, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCMUSIC_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 128 }, { -1, -1, 258 }, { 2, 0, 128 },
	{ -1, -1, 256 }, { 3, 0, 128 }, { 4, 0, 256 }, { 5, 0, 256 },
	{ 6, 0, 128 }, { 7, 1, 0 }, { 8, 1, 256 }, { 9, 1, 256 },
	{ 10, 1, 256 }, { 11, 2, 256 }, { 12, 2, 256 }, { 13, 2, 256 },
	{ 14, 3, 256 }, { 15, 3, 256 }, { 16, 7, 256 }, { 17, 7, 256 },
};
static const TImcSongOscillator IMCMUSIC_OscillatorList[] = {
	{ 9, 2, IMCSONGOSCTYPE_SQUARE, 0, -1, 255, 1, 2 },
	{ 8, 2, IMCSONGOSCTYPE_SQUARE, 0, -1, 255, 3, 4 },
	{ 8, 2, IMCSONGOSCTYPE_SQUARE, 0, -1, 0, 4, 4 },
	{ 8, 2, IMCSONGOSCTYPE_SQUARE, 0, -1, 228, 5, 4 },
	{ 8, 2, IMCSONGOSCTYPE_SQUARE, 0, -1, 142, 6, 7 },
	{ 8, 48, IMCSONGOSCTYPE_NOISE, 1, -1, 94, 4, 4 },
	{ 4, 15, IMCSONGOSCTYPE_SINE, 1, -1, 56, 4, 4 },
	{ 8, 48, IMCSONGOSCTYPE_SINE, 1, 6, 130, 4, 10 },
	{ 9, 0, IMCSONGOSCTYPE_SINE, 2, -1, 64, 4, 4 },
	{ 10, 0, IMCSONGOSCTYPE_SINE, 2, -1, 176, 4, 4 },
	{ 11, 0, IMCSONGOSCTYPE_SINE, 2, -1, 116, 4, 4 },
	{ 9, 0, IMCSONGOSCTYPE_SINE, 2, -1, 72, 4, 4 },
	{ 8, 0, IMCSONGOSCTYPE_SAW, 3, -1, 100, 4, 4 },
	{ 9, 0, IMCSONGOSCTYPE_SINE, 3, 12, 100, 4, 4 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 7, -1, 127, 4, 19 },
};
static const TImcSongEffect IMCMUSIC_EffectList[] = {
	{ 16891, 182, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 8 },
	{ 145, 0, 1, 0, IMCSONGEFFECTTYPE_LOWPASS, 4, 0 },
	{ 146, 140, 1, 0, IMCSONGEFFECTTYPE_RESONANCE, 4, 4 },
	{ 53, 0, 1653, 1, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 43, 0, 977, 1, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 4953, 1686, 1, 1, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 11 },
	{ 100, 0, 1, 1, IMCSONGEFFECTTYPE_LOWPASS, 12, 0 },
	{ 8382, 449, 1, 2, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 14 },
	{ 119, 179, 1, 2, IMCSONGEFFECTTYPE_RESONANCE, 15, 4 },
	{ 117, 0, 15034, 3, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 255, 0, 1, 3, IMCSONGEFFECTTYPE_HIGHPASS, 17, 0 },
	{ 128, 0, 8794, 7, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 255, 110, 1, 7, IMCSONGEFFECTTYPE_RESONANCE, 4, 4 },
	{ 227, 0, 1, 7, IMCSONGEFFECTTYPE_HIGHPASS, 4, 0 },
};
static unsigned char IMCMUSIC_ChannelVol[8] = { 81, 102, 20, 25, 100, 100, 100, 192 };
static const unsigned char IMCMUSIC_ChannelEnvCounter[8] = { 0, 9, 13, 16, 0, 0, 0, 18 };
static const bool IMCMUSIC_ChannelStopNote[8] = { true, false, true, true, false, false, false, true };
TImcSongData imcDataIMCMUSIC = {
	/*LEN*/ 0x17, /*ROWLENSAMPLES*/ 7517, /*ENVLISTSIZE*/ 18, /*ENVCOUNTERLISTSIZE*/ 20, /*OSCLISTSIZE*/ 18, /*EFFECTLISTSIZE*/ 14, /*VOL*/ 25,
	IMCMUSIC_OrderTable, IMCMUSIC_PatternData, IMCMUSIC_PatternLookupTable, IMCMUSIC_EnvList, IMCMUSIC_EnvCounterList, IMCMUSIC_OscillatorList, IMCMUSIC_EffectList,
	IMCMUSIC_ChannelVol, IMCMUSIC_ChannelEnvCounter, IMCMUSIC_ChannelStopNote };
ZL_SynthImcTrack imcMusic(&imcDataIMCMUSIC);

static const unsigned int IMCGRAB_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCGRAB_PatternData[] = {
	0x60, 0x30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCGRAB_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCGRAB_EnvList[] = {
	{ 0, 256, 204, 8, 16, 4, true, 255, },
	{ 0, 256, 209, 8, 16, 255, true, 255, },
	{ 64, 256, 261, 8, 15, 255, true, 255, },
	{ 0, 256, 3226, 8, 16, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCGRAB_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 256 }, { 2, 0, 256 }, { -1, -1, 256 },
	{ 3, 0, 256 },
};
static const TImcSongOscillator IMCGRAB_OscillatorList[] = {
	{ 6, 127, IMCSONGOSCTYPE_SINE, 0, -1, 206, 1, 2 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 0, -1, 186, 4, 3 },
	{ 7, 0, IMCSONGOSCTYPE_NOISE, 0, 0, 152, 3, 3 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static const TImcSongEffect IMCGRAB_EffectList[] = {
	{ 9906, 843, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 3 },
	{ 142, 58, 1, 0, IMCSONGEFFECTTYPE_RESONANCE, 3, 3 },
};
static unsigned char IMCGRAB_ChannelVol[8] = { 71, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCGRAB_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCGRAB_ChannelStopNote[8] = { false, false, false, false, false, false, false, false };
TImcSongData imcDataIMCGRAB = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 4, /*ENVCOUNTERLISTSIZE*/ 5, /*OSCLISTSIZE*/ 10, /*EFFECTLISTSIZE*/ 2, /*VOL*/ 100,
	IMCGRAB_OrderTable, IMCGRAB_PatternData, IMCGRAB_PatternLookupTable, IMCGRAB_EnvList, IMCGRAB_EnvCounterList, IMCGRAB_OscillatorList, IMCGRAB_EffectList,
	IMCGRAB_ChannelVol, IMCGRAB_ChannelEnvCounter, IMCGRAB_ChannelStopNote };

static const unsigned int IMCTHROW_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCTHROW_PatternData[] = {
	0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCTHROW_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCTHROW_EnvList[] = {
	{ 0, 256, 53, 8, 16, 255, true, 255, },
	{ 0, 256, 1743, 25, 15, 255, true, 255, },
	{ 144, 256, 108, 0, 24, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCTHROW_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 2 }, { 2, 0, 200 }, { -1, -1, 256 },
};
static const TImcSongOscillator IMCTHROW_OscillatorList[] = {
	{ 9, 150, IMCSONGOSCTYPE_SINE, 0, -1, 100, 1, 2 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static const TImcSongEffect IMCTHROW_EffectList[] = {
	{ 133, 0, 1653, 0, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 97, 105, 1, 0, IMCSONGEFFECTTYPE_RESONANCE, 3, 3 },
};
static unsigned char IMCTHROW_ChannelVol[8] = { 100, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCTHROW_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCTHROW_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
TImcSongData imcDataIMCTHROW = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 3307, /*ENVLISTSIZE*/ 3, /*ENVCOUNTERLISTSIZE*/ 4, /*OSCLISTSIZE*/ 8, /*EFFECTLISTSIZE*/ 2, /*VOL*/ 125,
	IMCTHROW_OrderTable, IMCTHROW_PatternData, IMCTHROW_PatternLookupTable, IMCTHROW_EnvList, IMCTHROW_EnvCounterList, IMCTHROW_OscillatorList, IMCTHROW_EffectList,
	IMCTHROW_ChannelVol, IMCTHROW_ChannelEnvCounter, IMCTHROW_ChannelStopNote };

static const unsigned int IMCGAMEOVER_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCGAMEOVER_PatternData[] = {
	0x64, 0x62, 0x60, 0x62, 0x60, 0x5B, 0x59, 0x57, 0x55, 0x54, 0x52, 0x50, 0, 0, 0, 0,
};
static const unsigned char IMCGAMEOVER_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCGAMEOVER_EnvList[] = {
	{ 0, 256, 92, 8, 16, 255, true, 255, },
	{ 0, 256, 14, 7, 15, 255, true, 255, },
	{ 118, 138, 1046, 8, 255, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCGAMEOVER_EnvCounterList[] = {
	{ 0, 0, 256 }, { -1, -1, 256 }, { 1, 0, 254 }, { 2, 0, 138 },
};
static const TImcSongOscillator IMCGAMEOVER_OscillatorList[] = {
	{ 8, 0, IMCSONGOSCTYPE_SQUARE, 0, -1, 100, 1, 2 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, 0, 72, 1, 3 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static unsigned char IMCGAMEOVER_ChannelVol[8] = { 100, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCGAMEOVER_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCGAMEOVER_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
TImcSongData imcDataIMCGAMEOVER = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 4033, /*ENVLISTSIZE*/ 3, /*ENVCOUNTERLISTSIZE*/ 4, /*OSCLISTSIZE*/ 9, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 100,
	IMCGAMEOVER_OrderTable, IMCGAMEOVER_PatternData, IMCGAMEOVER_PatternLookupTable, IMCGAMEOVER_EnvList, IMCGAMEOVER_EnvCounterList, IMCGAMEOVER_OscillatorList, NULL,
	IMCGAMEOVER_ChannelVol, IMCGAMEOVER_ChannelEnvCounter, IMCGAMEOVER_ChannelStopNote };
