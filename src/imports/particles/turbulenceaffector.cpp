#include "turbulenceaffector.h"
#include "particle.h"
#include <cmath>

TurbulenceAffector::TurbulenceAffector(QObject *parent) :
    ParticleAffector(parent), m_system(0), m_strength(10), m_lastT(0), m_frequency(1), m_size(10)
{
}

static qreal magnitude(qreal x, qreal y)
{
    return sqrt(x*x + y*y);
}

void TurbulenceAffector::ensureInit(ParticleData* d)
{
    if(m_system == d->p->m_system)
        return;
    m_system = d->p->m_system;
    m_field.resize(m_size);
    for(int i=0; i<m_size; i++)
        m_field[i].resize(m_size);
    m_spacing = QPointF(m_system->width()/m_size, m_system->height()/m_size);
    m_magSum = magnitude(m_spacing.x(), m_spacing.y())*2;
    //TODO: Don't crash when system size changes
}

void TurbulenceAffector::tickAdvance(ParticleData *d)
{
    if(d->pv.dt == m_lastT)
        return;
    m_lastT = d->pv.dt;
    for(int i=0; i<m_frequency; i++)
        mapUpdate();
}

void TurbulenceAffector::mapUpdate()
{
    QPoint pos(rand() % m_size, rand() % m_size);
    QPointF vector(m_strength  - (((qreal)rand() / RAND_MAX) * m_strength*2),
                   m_strength  - (((qreal)rand() / RAND_MAX) * m_strength*2));
    for(int i = 0; i < m_size; i++){
        for(int j = 0; j < m_size; j++){
            qreal dist = magnitude(i-pos.x(), j-pos.y());
            m_field[i][j] += vector/(dist + 1);
            if(magnitude(m_field[i][j].x(), m_field[i][j].y()) > m_strength){
                //Speed limit
                qreal theta = atan2(m_field[i][j].y(), m_field[i][j].x());
                m_field[i][j].setX(m_strength * cos(theta));
                m_field[i][j].setY(m_strength * sin(theta));
            }
        }
    }
}


bool TurbulenceAffector::affect(ParticleData *d, qreal dt)
{
    ensureInit(d);
    tickAdvance(d);
    qreal fx = 0.0;
    qreal fy = 0.0;
    QPointF pos = QPointF(d->curX(), d->curY());
    QPointF nodePos = QPointF(pos.x() / m_spacing.x(), pos.y() / m_spacing.y());
    QSet<QPair<int, int> > nodes;
    nodes << qMakePair((int)ceil(nodePos.x()), (int)ceil(nodePos.y()));
    nodes << qMakePair((int)ceil(nodePos.x()), (int)floor(nodePos.y()));
    nodes << qMakePair((int)floor(nodePos.x()), (int)ceil(nodePos.y()));
    nodes << qMakePair((int)floor(nodePos.x()), (int)floor(nodePos.y()));
    typedef QPair<int, int> intPair;
    foreach(const intPair &p, nodes){
        if(!QRect(0,0,m_size-1,m_size-1).contains(QPoint(p.first, p.second)))
            continue;
        qreal dist = magnitude(pos.x() - p.first*m_spacing.x(), pos.y() - p.second*m_spacing.y());
        fx += m_field[p.first][p.second].x() * ((m_magSum - dist)/m_magSum);//Proportionally weight nodes
        fy += m_field[p.first][p.second].y() * ((m_magSum - dist)/m_magSum);
    }
    if(!fx && !fy)
        return false;
    d->setInstantaneousSX(d->curSX()+ fx * dt);
    d->setInstantaneousSY(d->curSY()+ fy * dt);
    return true;
}

void TurbulenceAffector::reset(int systemIdx)
{
}